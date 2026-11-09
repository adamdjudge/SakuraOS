/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#include "io.h"

/* UART base addresses */
#define BASE_COM1   0x3f8
#define BASE_COM2   0x2f8

/* UART register offsets */
#define FIFO        0
#define IEN         1
#define DIVISOR_LO  0
#define DIVISOR_HI  1
#define INTID       2
#define FIFOCTL     2
#define LCR         3
#define MCR         4
#define LSR         5
#define MSR         6
#define SCRATCH     7

void uart_init(unsigned int baudrate)
{
    unsigned int divisor = 115200 / baudrate;

    out_byte_wait(BASE_COM1 + IEN, 0x00);      /* Disable interrupts */
    out_byte_wait(BASE_COM1 + LCR, 0x80);      /* Set DLAB */
    out_byte_wait(BASE_COM1 + DIVISOR_LO, divisor & 0xff);
    out_byte_wait(BASE_COM1 + DIVISOR_HI, (divisor >> 8) & 0xff);
    out_byte_wait(BASE_COM1 + LCR, 0x03);      /* 8b data, no parity, 1b stop */
    out_byte_wait(BASE_COM1 + FIFOCTL, 0x07);  /* Enable and clear FIFO */
}

void uart_puts(const char *s)
{
    while (*s) {
        while ((in_byte(BASE_COM1 + LSR) & 0x20) == 0)
            ;
        if (*s == '\n')
            out_byte(BASE_COM1 + FIFO, '\r');
        out_byte(BASE_COM1 + FIFO, *s++);
    }
}
