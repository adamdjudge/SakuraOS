/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: floppy.h
 */

#ifndef FLOPPY_H
#define FLOPPY_H

/* Floppy drive controller register I/O ports */
enum {
    FDC_SRA = 0x3f0,    /* Status register A */
    FDC_SRB = 0x3f1,    /* Status register B */
    FDC_DOR = 0x3f2,    /* Digital output register */
    FDC_MSR = 0x3f4,    /* Main status register (R) */
    FDC_DSR = 0x3f4,    /* Data rate select register (W) */
    FDC_FIFO = 0x3f5,   /* Command/data FIFO */
    FDC_DIR = 0x3f7,    /* Digital input register (R) */
    FDC_CCR = 0x3f7,    /* Configuration control register (W) */
};

/* FDC DOR register fields */
enum {
    DOR_DRIVE0 = 0x00,  /* Select drive 0 */
    DOR_DRIVE1 = 0x01,  /* Select drive 1 */
    DOR_DRIVE2 = 0x02,  /* Select drive 2 */
    DOR_DRIVE3 = 0x03,  /* Select drive 3 */

    DOR_ENABLE = 0x04,  /* Enable FDC, clear to reset */
    DOR_DMA = 0x08,     /* Enable DMA and IRQs */

    DOR_MOTOR0 = 0x10,  /* Drive 0 motor control */
    DOR_MOTOR1 = 0x20,  /* Drive 1 motor control */
    DOR_MOTOR2 = 0x40,  /* Drive 2 motor control */
    DOR_MOTOR3 = 0x80,  /* Drive 3 motor control */

    DOR_MOTOR_MASK = 0xf0,
};

/* FDC MSR register bitfields */
enum {
    MSR_SEEK0 = 0x01,   /* Indicates drive 0 is seeking */
    MSR_SEEK1 = 0x02,   /* Indicates drive 1 is seeking */
    MSR_SEEK2 = 0x04,   /* Indicates drive 2 is seeking */
    MSR_SEEK3 = 0x08,   /* Indicates drive 3 is seeking */
    MSR_BUSY = 0x10,    /* FDC busy with command */
    MSR_NDMA = 0x20,    /* Set when executing non-DMA transfer */
    MSR_DIO = 0x40,     /* FIFO data direction, set if read expected */
    MSR_RQM = 0x80,     /* FDC ready for FIFO exchange */
};

/* FDC command opcodes */
enum {
    FDC_READ_TRACK = 2,
    FDC_SPECIFY = 3,
    FDC_WRITE = 5,
    FDC_READ = 6,
    FDC_CALIBRATE = 7,
    FDC_SENSE_INTERRUPT = 8,
    FDC_VERSION = 16,
    FDC_CONFIGURE = 19,
    FDC_LOCK = 0x94,

    /* Modifier bits */
    FDC_SK = 0x20,
    FDC_MF = 0x40,
    FDC_MT = 0x80
};

#define FDC_VERSION_82077AA 0x90

/* Configure command parameter byte 2 bits */
enum {
    FDC_CONF_NPOLL = 0x10,  /* Disable polling mode */
    FDC_CONF_NFIFO = 0x20,  /* Disable FIFO buffering */
    FDC_CONF_SEEK = 0x40,   /* Enable implied seek before R/W */
};

/* DMA channel 2 registers */
enum {
    DMA2_ADDR = 0x04,
    DMA2_COUNT = 0x05,
    DMA2_PAGE = 0x81,
    DMA_MASK = 0x0a,
    DMA_MODE = 0x0b,
    DMA_FLIPFLOP = 0x0c,
};

#define SECTOR_SIZE 512

/**
 * Submit an I/O request to the floppy drive controller. Blocks the calling
 * task until the request is serviced, and returns whether the operation
 * completed successfully.
 */
bool floppy_rw(void *buf, uint8_t minor, int lba, int nblk, bool write);

#endif
