/**
 * The SakuraOS Bootloader
 * Copyright 2025 Adam Judge
 */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "stdint.h"

#define NULL (void*) 0

#define ABS(x) (x < 0 ? -x : x)
#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

uint8_t in_byte(uint16_t port);
void out_byte(uint16_t port, uint8_t val);

#endif
