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

uint8_t in_byte(uint16_t port);
uint8_t in_byte_wait(uint16_t port);
void out_byte(uint16_t port, uint8_t val);
void out_byte_wait(uint16_t port, uint8_t val);

void init_debug();
void __debug(const char *file, int line, const char *fmt, ...);
#define debug(...) __debug(__FILE__, __LINE__, __VA_ARGS__)

#endif
