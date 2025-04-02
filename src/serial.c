/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: serial.h
 */

#include <kernel.h>
#include <x86.h>
#include <serial.h>

void serial_init()
{
    out_byte_wait(BASE_COM1 + UART_IEN, 0x00);        /* Disable interrupts */
    out_byte_wait(BASE_COM1 + UART_LCR, 0x80);        /* Set DLAB */
    out_byte_wait(BASE_COM1 + UART_DIVISOR_LO, 0x0C); /* Set divisor to 12 */
    out_byte_wait(BASE_COM1 + UART_DIVISOR_HI, 0x00); /*   for 9600 baud */
    out_byte_wait(BASE_COM1 + UART_LCR, 0x03);        /* 8b data, no parity, 1b stop */
    out_byte_wait(BASE_COM1 + UART_FIFOCTL, 0x07);    /* Enable and clear FIFO */
    out_byte_wait(BASE_COM1 + UART_IEN, 0x01);        /* Enable receive interrupts */
}

void serial_write(uint8_t minor, char *buf, unsigned int length)
{
    while (length--) {
        while ((in_byte(BASE_COM1 + UART_LSR) & 0x20) == 0);
        if (*buf == '\n')
            out_byte(BASE_COM1 + UART_FIFO, '\r');
        out_byte(BASE_COM1 + UART_FIFO, *buf++);
    }
}
