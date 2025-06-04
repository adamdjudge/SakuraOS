/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#ifndef CHRDEV_H
#define CHRDEV_H

int readchr(dev_t dev, char *buf, unsigned int length);
int writechr(dev_t dev, char *buf, unsigned int length);

#endif
