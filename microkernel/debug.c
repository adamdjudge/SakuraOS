/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#include <stdarg.h>
#include "kernel.h"

extern void uart_puts(char *s);

static void printd(char **p, unsigned int v)
{
    if (v / 10)
        printd(p, v / 10);
    *(*p)++ = '0' + v % 10;
}

void vsprintf(char *buf, const char *fmt, va_list ap)
{
    char *s;
    int val;

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
            *buf++ = (char) va_arg(ap, int);
            break;

        case 's':
            s = va_arg(ap, char *);
            while (*s)
                *buf++ = *s++;
            break;

        case 'x': // This always prints 32-bit 8-digit hex
            s = "0123456789ABCDEF";
            val = va_arg(ap, int);
            for (int j = 28; j >= 0; j -= 4)
                *buf++ = s[(val >> j) & 0xf];
            break;

        case 'd':
            val = va_arg(ap, int);
            if (val < 0)
                *buf++ = '-';
            printd(&buf, ABS(val));
            ap++;
            break;

        case 'u':
            printd(&buf, va_arg(ap, unsigned int));
            break;

        default:
            *buf++ = '%';
            *buf++ = fmt[i];
        }
    }

    *buf = '\0';
}

void sprintf(char *buf, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
}

void __debug(const char *file, int line, const char *fmt, ...)
{
    char buf[128];
    va_list ap;

    sprintf(buf, "%s:%d: ", file, line);
    uart_puts(buf);

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);

    uart_puts(buf);
    uart_puts("\n");
}
