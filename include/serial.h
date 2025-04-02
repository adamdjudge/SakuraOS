/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: serial.h
 */

#ifndef SERIAL_H
#define SERIAL_H

/* UART base addresses */
#define BASE_COM1        0x3f8
#define BASE_COM2        0x2f8

/* UART register offsets */
#define UART_FIFO        0
#define UART_IEN         1
#define UART_DIVISOR_LO  0
#define UART_DIVISOR_HI  1
#define UART_INTID       2
#define UART_FIFOCTL     2
#define UART_LCR         3
#define UART_MCR         4
#define UART_LSR         5
#define UART_MSR         6
#define UART_SCRATCH     7

void serial_init();
void serial_write(uint8_t minor, char *buf, unsigned int length);

#endif
