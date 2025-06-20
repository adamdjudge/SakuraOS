/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: file.c
 */

#include <kernel.h>
#include <fs.h>
#include <sched.h>
#include <x86.h>
#include <chrdev.h>

static struct file files[NUM_FILES];
static mutex_t files_lock;

int open(char *path, unsigned int flags, unsigned int creat_mode)
{
    struct file *f;
    int fd, ret;

    if ((flags & O_ACCMODE) == O_INVALID_ACCMODE)
        return -EINVAL;

    mutex_lock(&files_lock);
    for (f = files; f < files + NUM_FILES; f++) {
        if (f->count == 0) {
            f->count++;
            break;
        }
    }
    mutex_unlock(&files_lock);
    if (f == files + NUM_FILES)
        return -ENFILE;

    mutex_lock(&proc->files_lock);
    for (fd = 0; fd < OPEN_MAX; fd++) {
        if (proc->files[fd] == 0) {
            proc->files[fd] = f;
            break;
        }
    }
    mutex_unlock(&proc->files_lock);
    if (fd == OPEN_MAX) {
        f->count = 0;
        return EMFILE;
    }

    /* TODO: create file if path not found and O_CREAT specified */
    ret = ilookup(&f->inode, path);
    if (ret) {
        proc->files[fd] = NULL;
        f->count = 0;
        return ret;
    }

    f->flags = flags;
    f->pos = 0;

    /* TODO: char dev driver open */

    return fd;
}

int close(int fd)
{
    struct file *f;

    if (fd < 0 || fd >= OPEN_MAX || proc->files[fd] == NULL)
        return -EBADF;
    f = proc->files[fd];
    proc->files[fd] = NULL;

    /* TODO: char dev driver close */

    iput(f->inode);
    dec_dword(&f->count);
    return 0;
}

int dup(int fd)
{
    struct file *f;
    int i;

    if (fd < 0 || fd >= OPEN_MAX || proc->files[fd] == NULL)
        return -EBADF;
    f = proc->files[fd];

    mutex_lock(&proc->files_lock);
    for (i = 0; i < OPEN_MAX; i++) {
        if (proc->files[i] == NULL) {
            proc->files[i] = f;
            inc_dword(&f->count);
            break;
        }
    }
    mutex_unlock(&proc->files_lock);

    return i == OPEN_MAX ? -EMFILE : i;
}

int read(int fd, char *buf, unsigned int length)
{
    struct file *f;
    bool setpos;
    int ret;

    if (fd < 0 || fd >= OPEN_MAX || proc->files[fd] == NULL)
        return -EBADF;
    f = proc->files[fd];

    if ((f->flags & O_ACCMODE) == O_WRONLY)
        return -EBADF;

    if (length > RW_MAX)
        length = RW_MAX;

    /* Don't set position for non-seekable streams, in which case we also don't
     * wait on a lock for the file; this allows signals to interrupt the read
     * on this "slow" device. */
    setpos = MODE_TYPE(f->inode->mode) != IFIFO &&
             MODE_TYPE(f->inode->mode) != IFCHR &&
             MODE_TYPE(f->inode->mode) != IFSOCK;

    if (setpos)
        mutex_lock(&f->lock);

    switch(MODE_TYPE(f->inode->mode)) {
    case IFCHR:
        ret = readchr(f->inode->zones[0], buf, length);
        break;
    case IFREG:
    case IFBLK:
        ret = iread(f->inode, buf, f->pos, length);
        break;
    default:
        ret = -ENOSYS;
        goto done;
    }

    if (setpos && ret >= 0)
        f->pos += ret;

done:
    if (setpos)
        mutex_unlock(&f->lock);
    return ret;
}

int write(int fd, char *buf, unsigned int length)
{
    struct file *f;
    bool setpos;
    int ret;

    if (fd < 0 || fd >= OPEN_MAX || proc->files[fd] == NULL)
        return -EBADF;
    f = proc->files[fd];

    if ((f->flags & O_ACCMODE) == O_RDONLY)
        return -EBADF;

    if (length > RW_MAX)
        length = RW_MAX;

    /* Don't set position for non-seekable streams, in which case we also don't
     * wait on a lock for the file; this allows signals to interrupt the write
     * on this "slow" device. */
    setpos = MODE_TYPE(f->inode->mode) != IFIFO &&
             MODE_TYPE(f->inode->mode) != IFCHR &&
             MODE_TYPE(f->inode->mode) != IFSOCK;

    if (setpos) {
        mutex_lock(&f->lock);
        if (f->flags & O_APPEND)
            f->pos = f->inode->size;
    }

    switch(MODE_TYPE(f->inode->mode)) {
    case IFCHR:
        ret = writechr(f->inode->zones[0], buf, length);
        break;
    case IFREG:
    case IFBLK:
        ret = iwrite(f->inode, buf, f->pos, length);
        break;
    default:
        ret = -ENOSYS;
        goto done;
    }

    if (setpos && ret >= 0)
        f->pos += ret;

done:
    if (setpos)
        mutex_unlock(&f->lock);
    return ret;
}
