/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: floppy.c
 */

/*
 * NOTE: At the moment this driver is terrible, as it only supports one floppy
 * drive and, even worse, there is no queueing, so if multiple threads are
 * trying to submit a command at once, they will happen essentially randomly
 * depending on which one happens to lock the mutex first. This also means the
 * head will move a lot more than necessary due to the randomness of the LBA of
 * each command, which hurts performance and wears out the drive faster.
 */

#include <kernel.h>
#include <fs.h>
#include <blkdev.h>
#include <exception.h>
#include <sched.h>
#include <mm.h>
#include <x86.h>
#include <floppy.h>

static struct geom floppy_geom = {
    .cyls = 80,
    .heads = 2,
    .sects = 18,
};

static bool got_irq;
static int cur_cyl;
static int motor_timer;
static mutex_t floppy_lock;

static uint8_t *dma_buffer = (uint8_t*) 0x1000;

void handle_floppy_irq()
{
    got_irq = true;
}

/* TODO: replace with scheduler function to block on an IRQ */
static void wait_irq()
{
    while (!got_irq)
        yield_thread();
    got_irq = false;
}

/* TODO: limited number of retries */
static void fifo_write(uint8_t data)
{
    uint8_t msr = 0;

    while (msr & MSR_RQM == 0)
        msr = in_byte(FDC_MSR);
    out_byte_wait(FDC_FIFO, data);
}

/* TODO: limited number of retries */
static uint8_t fifo_read()
{
    uint8_t msr = 0;

    while (msr & MSR_RQM == 0)
        msr = in_byte(FDC_MSR);
    return in_byte_wait(FDC_FIFO);
}

static void set_motor(int drive, bool setting)
{
    uint8_t dor = in_byte(FDC_DOR) & (DOR_ENABLE | DOR_DMA | DOR_MOTOR_MASK);

    if (setting)
        out_byte_wait(FDC_DOR, dor | drive | (DOR_MOTOR0 << drive));
    else
        out_byte_wait(FDC_DOR, dor & ~(DOR_MOTOR0 << drive));

    /* Let motor spin up. */
    if (setting && (dor & (DOR_MOTOR0 << drive)) == 0)
        sleep_thread(15);
}

void floppy_update_timer()
{
    if (motor_timer > 0 && --motor_timer == 0)
        set_motor(0, false);
}

static void sense_interrupt(uint8_t *st0, uint8_t *cyl)
{
    uint8_t res1, res2;

    fifo_write(FDC_SENSE_INTERRUPT);
    res1 = fifo_read();
    res2 = fifo_read();
    if (st0)
        *st0 = res1;
    if (cyl)
        *cyl = res2;
}

static void configure()
{
    fifo_write(FDC_CONFIGURE);
    fifo_write(0);
    fifo_write(FDC_CONF_SEEK | FDC_CONF_NPOLL | 8);
    fifo_write(0);
}

static void specify(uint8_t steprate, uint8_t loadt, uint8_t unloadt)
{
    fifo_write(FDC_SPECIFY);
    fifo_write((steprate << 4) | unloadt);
    fifo_write((loadt << 1) | 0); /* Don't set NDMA. */
}

static bool calibrate(uint8_t drive)
{
    uint8_t cyl;

    set_motor(drive, true);

    for (int i = 0; i < 5; i++) {
        fifo_write(FDC_CALIBRATE);
        fifo_write(drive);
        wait_irq();

        sense_interrupt(NULL, &cyl);
        if (cyl == 0) {
            /* Head made it back to cylinder 0, so we're done. */
            set_motor(drive, false);
            return true;
        }
    }

    /* Abort if retries fail. */
    set_motor(drive, false);
    return false;
}

/* DMA is handled here currently since this is the only driver that uses it for
   now, but there may be a general-purpose DMA driver in the future. */
static void start_dma(void *buf, int length, bool write)
{
    length--;
    out_byte_wait(DMA_MASK, 0x6);
    out_byte_wait(DMA_FLIPFLOP, 0xff);
    out_byte_wait(DMA2_ADDR, (uint32_t)buf & 0xff);
    out_byte_wait(DMA2_ADDR, ((uint32_t)buf >> 8) & 0xff);
    out_byte_wait(DMA2_PAGE, 0);
    out_byte_wait(DMA_FLIPFLOP, 0xff);
    out_byte_wait(DMA2_COUNT, length & 0xff);
    out_byte_wait(DMA2_COUNT, (length >> 8) & 0xff);

    if (write)
        out_byte_wait(DMA_MODE, 0x5a);
    else
        out_byte_wait(DMA_MODE, 0x56);
    out_byte_wait(DMA_MASK, 0x2);
}

static bool rw_cmd(uint8_t drive, struct chs *chs, int nblk, bool write)
{
    uint8_t st0, st1, st2;

    for (int i = 0; i < 3; i++) {
        start_dma(dma_buffer, nblk * 512, write);

        if (write)
            fifo_write(FDC_WRITE | FDC_MF | FDC_MT);
        else
            fifo_write(FDC_READ | FDC_MF | FDC_MT);
        fifo_write((chs->head << 2) | drive);
        fifo_write(chs->cyl);
        fifo_write(chs->head);
        fifo_write(chs->sect);
        fifo_write(2);
        fifo_write(floppy_geom.sects);
        fifo_write(0x1b);
        fifo_write(0xff);

        wait_irq();

        st0 = fifo_read();
        st1 = fifo_read();
        st2 = fifo_read();
        fifo_read();
        fifo_read();
        fifo_read();
        fifo_read();

        if ((st0 & 0xc0) == 0)
            return true;
    }

    printk("floppy: io error: %x %x %x\n", st0, st1, st2);
    return false;
}

static bool floppy_io(void *buffer, uint8_t drive, struct chs *chs, int nblk,
                      bool write)
{
    int rem_sect, cmd_nblk;
    bool ret = true;

    mutex_lock(&floppy_lock);
    motor_timer = 0; /* Ensure motor isn't turned off during operation */
    set_motor(drive, true);

    while (nblk > 0) {
        cur_cyl = chs->cyl;
 
        /* Thanks to multi-track mode, if we start on head 0, we can read/write
         * up to two whole tracks with a single command. Otherwise if starting
         * on head 1 we can read/write up to the end of the current track. */
        rem_sect = (chs->head == 1)
                   ? (floppy_geom.sects + 1 - chs->sect)
                   : (2 * floppy_geom.sects + 1 - chs->sect);
        cmd_nblk = MIN(rem_sect, nblk);

        printk("fd%d: %s block at CHS:%d-%d-%d\n", drive,
               write ? "write" : "read", chs->cyl, chs->head, chs->sect);

        if (write)
            memcpy(dma_buffer, buffer, cmd_nblk * 512);
        if (!rw_cmd(drive, chs, cmd_nblk, write)) {
            ret = false;
            goto end;
        }
        if (!write)
            memcpy(buffer, dma_buffer, cmd_nblk * 512);

        nblk -= cmd_nblk;
        buffer += cmd_nblk * 512;
        if (chs->head == 1) {
            chs->cyl++;
            chs->head = 0;
        }
        else
            chs->head = 1;
        chs->sect = 0;
    }

end:
    motor_timer = 200; /* Start countdown to shutoff. */
    mutex_unlock(&floppy_lock);
    return ret;
}

/**
 * Submit an I/O request to the floppy drive controller. Blocks the calling
 * task until the request is serviced, and returns whether the operation
 * completed successfully.
 */
bool floppy_rw(void *buf, uint8_t minor, int lba, int nblk, bool write)
{
    struct chs chs;

    if (minor >= 4)
        return false;
    if (lba + nblk >= floppy_geom.cyls * floppy_geom.heads * floppy_geom.sects)
        return false;

    lba_to_chs(&floppy_geom, lba, &chs);
    return floppy_io(buf, minor, &chs, nblk, write);
}

void floppy_init()
{
    uint32_t addr;
    uint8_t ver;

    /* Map a buffer for floppy DMA, using absolute placement since the DMA
     * controller can only access the low 16 MiB. We make room for the largest
     * possible single transfer, which is 2 whole tracks or 36 sectors. */
    for (addr = (uint32_t)dma_buffer;
         addr < (uint32_t)dma_buffer + 2*18*512;
         addr += PAGE_SIZE)
    {
        if (!map_page(addr, addr, PAGE_WRITABLE))
            panic("failed to map DMA buffer");
    }

    out_byte_wait(FDC_DOR, 0);
    out_byte_wait(FDC_DOR, DOR_ENABLE | DOR_DMA);
    wait_irq();

    sense_interrupt(NULL, NULL);
    sense_interrupt(NULL, NULL);
    sense_interrupt(NULL, NULL);
    sense_interrupt(NULL, NULL);

    fifo_write(FDC_VERSION);
    ver = fifo_read();
    if (ver == FDC_VERSION_82077AA)
        printk("floppy: controller is 82077AA\n");
    else
        printk("floppy: controller is not 82077AA, version is %d\n", ver);

    out_byte_wait(FDC_CCR, 0); /* 500 kb/s transfer speed for 1.44m disks */
    configure();
    specify(8, 5, 0);
    if (calibrate(0))
        printk("floppy: init successful\n");
    else
        printk("floppy: init failed\n");
}
