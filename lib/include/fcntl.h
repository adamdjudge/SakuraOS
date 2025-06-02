/**
 * The SakuraOS Standard Library
 * Copyright 2025 Adam Judge
 */

#ifndef _FCNTL_H
#define _FCNTL_H

/*
 * File access modes used by open() and fcntl().
 */
enum {
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
};

/*
 * File access mode mask.
 */
#define O_ACCMODE  0x3

/*
 * File status flags used by open() and fcntl().
 */
#define O_APPEND   (1<<2)
#define O_CLOEXEC  (1<<3)
#define O_CREAT    (1<<4)
#define O_DSYNC    (1<<5)
#define O_EXCL     (1<<6)
#define O_NOCTTY   (1<<7)
#define O_NONBLOCK (1<<8)
#define O_SYNC     (1<<9)
#define O_TRUNC    (1<<10)

int open(const char *, int, ...);

#endif
