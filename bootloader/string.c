/**
 * The SakuraOS Bootloader
 * Copyright 2026 Adam Judge
 */

#include "bootloader.h"
#include "string.h"
#include "console.h"

void memcpy(char *dest, char *src, unsigned int len)
{
    while (len--)
        *dest++ = *src++;
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

void printf(const char *fmt, ...)
{
    char buf[128];

    __vsprintf(buf, fmt, &fmt + 1);
    for (int i = 0; buf[i]; i++)
        console_putc(buf[i]);
}
