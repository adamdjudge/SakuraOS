/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stdbool.h>

#define NULL (void*) 0

#define ABS(x) (x < 0 ? -x : x)
#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

void vsprintf(char *buf, const char *fmt, void *argp);
void printk(const char *fmt, ...);

void memset(void *s, char c, unsigned int n);
void memcpy(void *dest, void *src, unsigned int n);

typedef int mutex_t;

void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

void panic(const char *msg);

enum {
    E2BIG = 1,
    EACCESS,
    EAGAIN,
    EBADF,
    EBADFD,
    EBADR,
    EBADRQC,
    EBADSLT,
    EBUSY,
    ECHILD,
    EEXIST,
    EFAULT,
    EFBIG,
    EHWPOISON,
    EINPROGRESS,
    EINTR,
    EINVAL,
    EIO,
    EISDIR,
    EISNAM,
    ELIBACC,
    ELIBBAD,
    ELIBMAX,
    ELIBSCN,
    ELIBEXEC,
    ELNRANGE,
    ELOOP,
    EMFILE,
    EMLINK,
    ENAMETOOLONG,
    ENFILE,
    ENODEV,
    ENOENT,
    ENOEXEC,
    ENOMEDIUM,
    ENOMEM,
    ENOSPC,
    ENOSYS,
    ENOTBLK,
    ENOTDIR,
    ENOTEMPTY,
    ENOTSUP,
    ENOTTY,
    EOVERFLOW,
    EPERM,
    EPIPE,
    ERANGE,
    ERESTART,
    EROFS,
    ESPIPE,
    ESRCH,
    ETXTBSY,
    EUSERS,
    EWOULDBLOCK,
};

#endif
