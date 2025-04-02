/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: buffer.c
 */

#include <kernel.h>
#include <fs.h>
#include <blkdev.h>
#include <buffer.h>
#include <sched.h>

static struct buffer buffers[NUM_BUFFERS];
static mutex_t buffers_lock;

static void lockbuf(struct buffer *b)
{
    while (b->flags & BUF_LOCK)
        yield_thread();
    b->flags |= BUF_LOCK;
}

struct buffer *getbuf(dev_t dev, int block)
{
    struct buffer *b, *buf;

repeat:
    mutex_lock(&buffers_lock);

    buf = buffers;
    for (b = buffers + 1; b < buffers + NUM_BUFFERS; b++) {
        if (b->dev == dev && b->block == block) {
            lockbuf(b);
            mutex_unlock(&buffers_lock);
            b->time = jiffies();
            return b;
        }
        
        if (b->time <= buf->time && !(b->flags & BUF_LOCK))
            buf = b;
    }
    if (buf->flags & BUF_LOCK) {
        // All buffers are locked, so wait until one is free
        mutex_unlock(&buffers_lock);
        yield_thread();
        goto repeat;
    }

    buf->flags |= BUF_LOCK;
    mutex_unlock(&buffers_lock);

    if (buf->flags & BUF_DIRTY) {
        block_rw(WRITE, buf);
        buf->flags &= ~BUF_DIRTY;
    }
    buf->flags &= ~BUF_UPTODATE;
    buf->dev = dev;
    buf->block = block;
    buf->time = jiffies();
    return buf;
}

void relbuf(struct buffer *b)
{
    b->flags &= ~BUF_LOCK;
}
