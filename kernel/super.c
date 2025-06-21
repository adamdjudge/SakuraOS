/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: super.c
 */

#include <kernel.h>
#include <fs.h>
#include <buffer.h>
#include <blkdev.h>

static struct superblock supers[NUM_SUPERS];
static spinlock_t supers_lock;

int mount(dev_t dev, struct inode **ip)
{
    struct superblock *s;
    struct buffer *b;
    int ret;

    spin_lock(&supers_lock);

    for (s = supers; s < supers + NUM_SUPERS; s++) {
        if (s->dev == 0)
            break;
    }
    if (s == supers + NUM_SUPERS) {
        spin_unlock(&supers_lock);
        return -1; // too many mounted volumes
    }

    s->dev = dev;
    spin_unlock(&supers_lock);

    b = readblk(dev, 1);
    if (!b) {
        printk("mount: failed to read superblock (dev %d:%d)\n",
               MAJOR(dev), MINOR(dev));
        s->dev = 0;
        return -1; // io error
    }
    memcpy(s, b->data, SB_SIZE);
    relbuf(b);

    if (s->magic != MINIX_14_MAGIC) {
        printk("mount: invalid volume (dev %d:%d)\n", MAJOR(dev), MINOR(dev));
        s->dev = 0;
        return -1; // invalid volume
    }

    ret = iget(ip, s, 1);
    if (ret < 0) {
        printk("mount: failed to read root inode (dev %d:%d)\n",
               MAJOR(dev), MINOR(dev));
        s->dev = 0;
        return ret;
    }

    printk("Mounted volume on dev %d:%d\n", MAJOR(dev), MINOR(dev));
    return 0;
}
