/**
 * The SakuraOS Bootloader
 * Copyright 2025 Adam Judge
 */

#include "bootloader.h"
#include "console.h"
#include "string.h"

struct memrange {
    uint32_t base;
    uint32_t _resvd1;
    uint32_t size;
    uint32_t _resvd2;
    uint32_t type;
};

extern struct memrange memtab[];

void main()
{
    console_init();
    printf("=============================");
    printf(" SakuraOS Bootloader ");
    printf("==============================");

    printf("Memory map from BIOS:\n");
    for (struct memrange *mr = memtab; mr->size != 0; mr++)
        printf("  %s 0x%x - 0x%x (%uk)\n",
               mr->type == 1 ? "free" : "resv", mr->base,
               mr->base + mr->size - 1, mr->size / 1024);
}

uint8_t in_byte(uint16_t port)
{
    uint8_t val;
    __asm__ __volatile__("inb %%dx, %%al" : "=a" (val) : "d" (port));
    return val;
}

void out_byte(uint16_t port, uint8_t val)
{
    __asm__ __volatile__("outb %%al, %%dx" :: "a" (val), "d" (port));
}
