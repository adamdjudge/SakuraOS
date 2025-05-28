/**
 * The SakuraOS Standard Library
 * Copyright 2025 Adam Judge
 */

#ifndef _ERRNO_H
#define _ERRNO_H

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

extern int errno;

#endif
