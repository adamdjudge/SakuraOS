/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: blkdev.c
 */

#include <kernel.h>
#include <fs.h>
#include <blkdev.h>
#include <buffer.h>

extern bool floppy_rw(void *buf, uint8_t minor, int lba, int nblk, bool write);

bool block_rw(int rw, struct buffer *b)
{
    if (rw != READ && rw != WRITE)
        return false;

    switch (MAJOR(b->dev)) {
    case 2:
        return floppy_rw(b->data, MINOR(b->dev), 2*b->block, 2, rw);
    default:
        printk("block_rw: attempted %s on invalid device %d:%d\n",
               rw ? "write" : "read", MAJOR(b->dev), MINOR(b->dev));
        return false;
    }
}

struct buffer *readblk(dev_t dev, unsigned int blk)
{
    struct buffer *b = getbuf(dev, blk);
    if ((b->flags & BUF_UPTODATE) == 0) {
        if (!block_rw(READ, b)) {
            relbuf(b);
            return NULL;
        }
        b->flags |= BUF_UPTODATE;
    }
    return b;
}
