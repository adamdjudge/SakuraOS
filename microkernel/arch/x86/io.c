/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#include "io.h"

uint8_t in_byte(uint16_t port)
{
    uint8_t val;
    __asm__ __volatile__("inb %%dx, %%al" : "=a" (val) : "d" (port));
    return val;
}

uint8_t in_byte_wait(uint16_t port)
{
    int count = 10000;
    while (count)
        count--;

    return in_byte(port);
}

void out_byte(uint16_t port, uint8_t val)
{
    __asm__ __volatile__("outb %%al, %%dx" :: "a" (val), "d" (port));
}

void out_byte_wait(uint16_t port, uint8_t val)
{
    int count = 10000;
    while (count)
        count--;

    out_byte(port, val);
}
