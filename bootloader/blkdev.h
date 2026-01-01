/**
 * The SakuraOS Bootloader
 * Copyright 2026 Adam Judge
 */

#ifndef BLKDEV_H
#define BLKDEV_H

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

#endif
