/**
 * The SakuraOS Bootloader
 * Copyright 2025 Adam Judge
 */

#include "bootloader.h"
#include "string.h"
#include "blkdev.h"
#include "floppy.h"

static struct geom floppy_geom = {
    .cyls = 80,
    .heads = 2,
    .sects = 18,
};

static bool got_irq;
static int motor_timer;
static void *dma_buffer = (void *)0x1000;

void handle_floppy_irq()
{
    got_irq = true;
}

static void wait_irq()
{
    while (!got_irq);
    got_irq = false;
}

/* TODO: limited number of retries */
static void fifo_write(uint8_t data)
{
    uint8_t msr = 0;

    while ((msr & MSR_RQM) == 0)
        msr = in_byte(FDC_MSR);
    out_byte_wait(FDC_FIFO, data);
}

/* TODO: limited number of retries */
static uint8_t fifo_read()
{
    uint8_t msr = 0;

    while ((msr & MSR_RQM) == 0)
        msr = in_byte(FDC_MSR);
    return in_byte_wait(FDC_FIFO);
}

static void set_motor(int drive, bool setting)
{
    uint8_t dor;

    motor_timer = 0;
    dor = in_byte(FDC_DOR) & (DOR_ENABLE | DOR_DMA | DOR_MOTOR_MASK);

    if (setting)
        out_byte_wait(FDC_DOR, dor | drive | (DOR_MOTOR0 << drive));
    else
        out_byte_wait(FDC_DOR, dor & ~(DOR_MOTOR0 << drive));

    /* Let motor spin up. */
    if (setting && (dor & (DOR_MOTOR0 << drive)) == 0)
        delay(200);
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

void floppy_init()
{
    uint8_t ver;

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
        printf("floppy: controller is 82077AA\n");
    else
        printf("floppy: controller is not 82077AA, version is %d\n", ver);

    out_byte_wait(FDC_CCR, 0); /* 500 kb/s transfer speed for 1.44m disks */
    configure();
    specify(8, 5, 0);
    if (calibrate(0))
        printf("floppy: init successful\n");
    else
        printf("floppy: init failed\n");
}

static bool rw_cmd(uint8_t drive, struct chs *chs, bool write)
{
    uint8_t st0, st1, st2;
    printf("fd%d: read block at CHS:%d-%d-%d\n", drive, chs->cyl, chs->head, chs->sect);

    for (int i = 0; i < 3; i++) {
        start_dma(dma_buffer, 1024, write);

        fifo_write((write ? FDC_WRITE : FDC_READ) | FDC_MF | FDC_MT);
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

    printf("floppy: io error: %x %x %x\n", st0, st1, st2);
    return false;
}

bool floppy_read_block(void *buf, uint8_t minor, unsigned int lba)
{
    bool ret;
    struct chs chs;

    if (minor >= 4)
        return false;
    if (lba + 2 >= floppy_geom.cyls * floppy_geom.heads * floppy_geom.sects)
        return false;
    lba_to_chs(&floppy_geom, lba, &chs);

    set_motor(minor, true);
    ret = rw_cmd(minor, &chs, false);
    if (ret)
        memcpy(buf, dma_buffer, 1024);
    motor_timer = 2000; /* Start countdown to motor shutoff */
    return ret;
}
