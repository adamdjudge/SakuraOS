/**
 * The SakuraOS Bootloader
 * Copyright 2026 Adam Judge
 */

#include "bootloader.h"
#include "string.h"
#include "console.h"
#include "floppy.h"

struct memrange {
    uint32_t base;
    uint32_t _resvd1;
    uint32_t size;
    uint32_t _resvd2;
    uint32_t type;
};

extern struct memrange memtab[];

extern void interrupt_init();

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
    
    interrupt_init();
    floppy_init();

    char buffer[1024];
    floppy_read_block(buffer, 0, 0);
    for (int i = 0; i < 512; i++)
        console_putc(buffer[i]);

    for (;;) {
        delay(1000);
        printf("timer done\n");
    }
}

uint8_t in_byte(uint16_t port)
{
    uint8_t val;
    __asm__ __volatile__("inb %%dx, %%al" : "=a" (val) : "d" (port));
    return val;
}

uint8_t in_byte_wait(uint16_t port)
{
    uint8_t val = in_byte(port);
    int count = 10000;
    while (count)
        count--;
    return val;
}

void out_byte(uint16_t port, uint8_t val)
{
    __asm__ __volatile__("outb %%al, %%dx" :: "a" (val), "d" (port));
}

void out_byte_wait(uint16_t port, uint8_t val)
{
    out_byte(port, val);
    int count = 10000;
    while (count)
        count--;
}
