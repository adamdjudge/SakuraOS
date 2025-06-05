/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#include <kernel.h>
#include <fs.h>

extern int tty_read(uint8_t minor, char *buf, unsigned int length);

int readchr(dev_t dev, char *buf, unsigned int length)
{
    switch(MAJOR(dev)) {
    case 4:
        return tty_read(MINOR(dev), buf, length);
    default:
        return -ENODEV;
    }
}

extern int tty_write(uint8_t minor, char *buf, unsigned int length);

int writechr(dev_t dev, char *buf, unsigned int length)
{
    switch(MAJOR(dev)) {
    case 4:
        return tty_write(MINOR(dev), buf, length);
    default:
        return -ENODEV;
    }
}
