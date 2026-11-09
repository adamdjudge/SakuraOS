/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#ifndef KERNEL_H
#define KERNEL_H

#define NULL (void*) 0

#define ABS(x) (x < 0 ? -x : x)
#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

/* Kernel error codes */
enum {
    SUCCESS,
    ERR_INVALID,
    ERR_BAD_PID,
    ERR_OUT_OF_MEMORY,
    ERR_NO_FREE_TCB,
};

void __debug(const char *file, int line, const char *fmt, ...);
#define debug(...) __debug(__FILE__, __LINE__, __VA_ARGS__)

#endif
