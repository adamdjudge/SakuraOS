/**
 * The SakuraOS Bootloader
 * Copyright 2026 Adam Judge
 */

#include "bootloader.h"
#include "string.h"
#include "console.h"
#include "floppy.h"
#include "fs.h"

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
    if (!fs_init()) {
        printf("Boot failed: could not initialize filesystem\n");
        return;
    }

    struct inode test;
    if (!get_bootdir_inode(&test, "test.txt")) {
        printf("failed to load test.txt inode\n");
        return;
    }

    static char buffer[4096];
    if (!fileread(&test, buffer, 0, test.size)) {
        printf("failed to read file\n");
        return;
    }
    
    for (int i = 0; buffer[i]; i++)
        console_putc(buffer[i]);
    printf("\nDone reading file: %d bytes\n", test.size);
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
