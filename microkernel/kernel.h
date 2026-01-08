/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#ifndef KERNEL_H
#define KERNEL_H

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

struct segment {
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t size;
    uint32_t flags;
};

#define MAX_SEGMENTS 4

struct module {
    uint32_t start_addr;
    struct segment segments[MAX_SEGMENTS];
};

#define MAX_MEMRANGES 8
#define MAX_MODULES 16

/* Kernel Parameters Block */
struct kpb {
    struct memrange memtab[MAX_MEMRANGES];
    struct module modules[MAX_MODULES];
};

uint8_t in_byte(uint16_t port);
uint8_t in_byte_wait(uint16_t port);
void out_byte(uint16_t port, uint8_t val);
void out_byte_wait(uint16_t port, uint8_t val);

void __debug(const char *file, int line, const char *fmt, ...);
#define debug(...) __debug(__FILE__, __LINE__, __VA_ARGS__)

#endif
