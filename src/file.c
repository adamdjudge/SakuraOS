/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: file.c
 */

#include <kernel.h>
#include <fs.h>

static struct file files[NUM_FILES];
static mutex_t files_lock;

int open(struct file **fp, char *path, unsigned int mode, unsigned int flags)
{
    struct file *f;
    int ret;

    if (mode > O_RDWR)
        return -1; /* invalid */

    mutex_lock(&files_lock);
    for (f = files; f < files + NUM_FILES; f++) {
        if (f->count == 0) {
            f->count++;
            break;
        }
    }
    mutex_unlock(&files_lock);
    if (f == files + NUM_FILES)
        return -1; /* too many open files */

    ret = ilookup(&f->inode, path);
    if (ret)
        return ret;

    f->mode = mode;
    f->flags = flags;
    f->pos = 0;

    /* TODO: char dev driver open */

    *fp = f;
    return 0;
}

void close(struct file *fp)
{
    /* TODO: char dev driver close */

    iput(fp->inode);
    mutex_lock(&fp->lock);
    fp->count--;
    mutex_unlock(&fp->lock);
}

int read(struct file *fp, char *buf, unsigned int length)
{
    int ret;

    if (fp->mode == O_WRONLY)
        return -1; /* not opened for reading */
    if (fp->pos >= fp->inode->size)
        return 0;

    ret = iread(fp->inode, buf, fp->pos, length);
    if (ret > 0)
        fp->pos += ret;
    return ret;
}
