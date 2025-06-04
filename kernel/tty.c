/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#include <kernel.h>
#include <console.h>

#define NUM_TTYS 3

struct tty {
    mutex_t lock;
    int flags;
    unsigned int head;
    unsigned int tail;
    char buffer[256];
};

struct tty ttys[NUM_TTYS];

int tty_write(uint8_t minor, char *buf, unsigned int length)
{
    unsigned int i;
    int ret;

    if (minor >= NUM_TTYS)
        return -ENODEV;

    mutex_lock(&ttys[minor].lock);
    if (minor == 0) {
        for (i = 0; i < length; i++)
            console_putc(buf[i]);
        ret = length;
    } else {
        ret = -ENODEV;
    }
    mutex_unlock(&ttys[minor].lock);
    return ret;
}
