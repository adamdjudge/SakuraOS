/**
 * The SakuraOS Bootloader
 * Copyright 2026 Adam Judge
 */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>
#include <stdbool.h>

#define NULL (void*) 0

#define ABS(x) (x < 0 ? -x : x)
#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

struct memrange {
    uint32_t base;
    uint32_t size;
    uint32_t type;
};

struct module {
    uint32_t start_addr;
    struct {
        uint32_t pbase;
        uint32_t vbase;
        uint32_t size;
        uint32_t flags;
    } segments[4];
};

/* Kernel Parameters Block */
struct kpb {
    struct memrange memtab[8];
    struct module modules[16];
};

extern struct kpb kpb;

uint8_t in_byte(uint16_t port);
uint8_t in_byte_wait(uint16_t port);
void out_byte(uint16_t port, uint8_t val);
void out_byte_wait(uint16_t port, uint8_t val);

void delay(unsigned int ms);

#endif
