/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: blkdev.h
 */

#ifndef BLKDEV_H
#define BLKDEV_H

enum {
    READ,
    WRITE
};

/* Block device geometry */
struct geom {
    int cyls;
    int heads;
    int sects;
};

/* Cylinder-head-sector block address */
struct chs {
    int cyl;
    int head;
    int sect;
};

/**
 * Given drive geometry, convert a linear block address to CHS address.
 */
static inline void lba_to_chs(struct geom *geom, int lba, struct chs *chs)
{
    chs->cyl = lba / (geom->heads * geom->sects);
    chs->head = (lba % (geom->heads * geom->sects)) / geom->sects;
    chs->sect = (lba % (geom->heads * geom->sects)) % geom->sects + 1;
}

struct buffer *readblk(dev_t dev, unsigned int blk);
bool block_rw(int rw, struct buffer *b);

#endif
