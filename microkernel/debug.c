/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#include "kernel.h"

/* UART base addresses */
#define BASE_COM1        0x3f8
#define BASE_COM2        0x2f8

/* UART register offsets */
#define UART_FIFO        0
#define UART_IEN         1
#define UART_DIVISOR_LO  0
#define UART_DIVISOR_HI  1
#define UART_INTID       2
#define UART_FIFOCTL     2
#define UART_LCR         3
#define UART_MCR         4
#define UART_LSR         5
#define UART_MSR         6
#define UART_SCRATCH     7

void init_debug()
{
    out_byte_wait(BASE_COM1 + UART_IEN, 0x00);        /* Disable interrupts */
    out_byte_wait(BASE_COM1 + UART_LCR, 0x80);        /* Set DLAB */
    out_byte_wait(BASE_COM1 + UART_DIVISOR_LO, 0x0C); /* Set divisor to 12 */
    out_byte_wait(BASE_COM1 + UART_DIVISOR_HI, 0x00); /*   for 9600 baud */
    out_byte_wait(BASE_COM1 + UART_LCR, 0x03);        /* 8b data, no parity, 1b stop */
    out_byte_wait(BASE_COM1 + UART_FIFOCTL, 0x07);    /* Enable and clear FIFO */
    out_byte_wait(BASE_COM1 + UART_IEN, 0x01);        /* Enable receive interrupts */
}

static void printd(char **p, unsigned int v)
{
    if (v / 10)
        printd(p, v / 10);
    *(*p)++ = '0' + v % 10;
}

static void __vsprintf(char *buf, const char *fmt, void *argp)
{
    int *ap = argp;
    char *s;
    
    for (int i = 0; fmt[i]; i++) {
        if (fmt[i] != '%') {
            *buf++ = fmt[i];
            continue;
        }
        switch (fmt[++i]) {
        case '%':
            *buf++ = '%';
            break;
        case 'c':
            *buf++ = (char) *ap++;
            break;
        case 's':
            s = (char*) *ap++;
            while (*s)
                *buf++ = *s++;
            break;
        case 'x': // NOTE: This always prints 32-bit 8-digit hex
            s = "0123456789ABCDEF";
            for (int j = 28; j >= 0; j -= 4)
                *buf++ = s[(*ap >> j) & 0xf];
            ap++;
            break;
        case 'd':
            if (*ap < 0)
                *buf++ = '-';
            printd(&buf, ABS(*ap));
            ap++;
            break;
        case 'u':
            printd(&buf, (unsigned int) *ap++);
            break;
        default:
            *buf++ = '%';
            *buf++ = fmt[i];
        }
    }
    *buf = '\0';
}

void vsprintf(char *buf, const char *fmt, ...)
{
    __vsprintf(buf, fmt, &fmt + 1);
}

static void write_serial(const char *buf)
{
    while (*buf) {
        while ((in_byte(BASE_COM1 + UART_LSR) & 0x20) == 0);
        if (*buf == '\n')
            out_byte(BASE_COM1 + UART_FIFO, '\r');
        out_byte(BASE_COM1 + UART_FIFO, *buf++);
    }
}

void __debug(const char *file, int line, const char *fmt, ...)
{
    char buf[128];

    vsprintf(buf, "%s:%d: ", file, line);
    write_serial(buf);
    __vsprintf(buf, fmt, &fmt + 1);
    write_serial(buf);
    write_serial("\n");
}
