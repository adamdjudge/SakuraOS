/**
 * The SakuraOS Bootloader
 * Copyright 2026 Adam Judge
 */

#include "bootloader.h"
#include "string.h"
#include "console.h"
#include "floppy.h"
#include "fs.h"

struct bios_memrange {
    uint32_t base;
    uint32_t _resvd1;
    uint32_t size;
    uint32_t _resvd2;
    uint32_t type;
};

extern struct bios_memrange memtab[];

extern void interrupt_init();
extern uint32_t load_kernel(char *name);

struct kpb kpb;

uint32_t main()
{
    console_init();
    printf("=============================");
    printf(" SakuraOS Bootloader ");
    printf("==============================");

    memset(&kpb, 0, sizeof(kpb));

    printf("Memory map from BIOS:\n");
    struct memrange *kpb_mr = kpb.memtab;
    for (struct bios_memrange *mr = memtab; mr->size != 0; mr++) {
        printf("  %s 0x%x - 0x%x (%uk)\n",
               mr->type == 1 ? "free" : "resv", mr->base,
               mr->base + mr->size - 1, mr->size / 1024);
        
        if (mr->base >= 0x100000) {
            kpb_mr->base = mr->base;
            kpb_mr->size = mr->size;
            kpb_mr->type = mr->type;
            kpb_mr++;
        }
    }
    
    interrupt_init();
    floppy_init();
    if (!fs_init()) {
        printf("Boot failed: could not initialize filesystem\n");
        return 0;
    }

    return load_kernel("kernel");
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
