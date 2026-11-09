/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#ifndef IO_H
#define IO_H

#include <stdint.h>

uint8_t in_byte(uint16_t port);
uint8_t in_byte_wait(uint16_t port);
void out_byte(uint16_t port, uint8_t val);
void out_byte_wait(uint16_t port, uint8_t val);

#endif
