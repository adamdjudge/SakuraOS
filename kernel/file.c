/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: file.c
 */

#include <kernel.h>
#include <fs.h>
#include <sched.h>
#include <x86.h>

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

int read(int fd, char *buf, unsigned int length)
{
    struct file *f;
    int ret;

    if (fd < 0 || fd >= OPEN_MAX || proc->files[fd] == NULL)
        return -EBADF;
    f = proc->files[fd];

    if ((f->flags & O_ACCMODE) == O_WRONLY)
        return -EBADF;

    mutex_lock(&f->lock);
    ret = iread(f->inode, buf, f->pos, length);
    f->pos += ret;
    mutex_unlock(&f->lock);
    return ret;
}

int write(int fd, char *buf, unsigned int length)
{
    struct file *f;
    int ret;

    if (fd < 0 || fd >= OPEN_MAX || proc->files[fd] == NULL)
        return -EBADF;
    f = proc->files[fd];

    if ((f->flags & O_ACCMODE) == O_RDONLY)
        return -EBADF;

    mutex_lock(&f->lock);
    if (f->flags & O_APPEND)
        f->pos = f->inode->size;
    ret = iwrite(f->inode, buf, f->pos, length);
    f->pos += ret;
    mutex_unlock(&f->lock);
    return ret;
}
