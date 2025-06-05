/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: inode.c
 */

#include <kernel.h>
#include <fs.h>
#include <blkdev.h>
#include <buffer.h>
#include <chrdev.h>
#include <sched.h>
#include <x86.h>

/* Root directory of filesystem tree */
struct inode *g_root_dir;

/* Internal inode cache */
static struct inode inodes[NUM_INODES];
static mutex_t inodes_lock;

int iget(struct inode **ip, struct superblock *s, unsigned int inum)
{
    struct inode *i;
    struct buffer *b;

    if (inum == 0 || inum > s->ninodes)
        return -1; // Index out of range

    mutex_lock(&inodes_lock);
    for (i = inodes; i < inodes + NUM_INODES; i++) {
        if (i->dev == s->dev && i->inum == inum) {
            idup(i);
            mutex_unlock(&inodes_lock);
            *ip = i;
            return 0;
        }
    }
    for (i = inodes; i < inodes + NUM_INODES; i++) {
        if (i->count == 0)
            break;
    }
    if (i == inodes + NUM_INODES) {
        mutex_unlock(&inodes_lock);
        return -ENFILE;
    }

    mutex_lock(&i->lock);
    i->dev = s->dev;
    i->inum = inum;
    i->count = 1;
    i->super = s;
    i->mount = NULL;
    mutex_unlock(&inodes_lock);

    b = readblk(s->dev, 2 + s->imap_blocks + s->zmap_blocks
                          + (inum - 1) / INODES_PER_BLOCK);
    if (!b) {
        i->count = 0;
        printk("iget: failed to read inode block\n");
        mutex_unlock(&i->lock);
        return -EIO;
    }
    memcpy(i, b->data + ((inum - 1) % INODES_PER_BLOCK) * INODE_SIZE,
           INODE_SIZE);
    relbuf(b);

    mutex_unlock(&i->lock);
    *ip = i;
    return 0;
}

struct inode *idup(struct inode *i)
{
    inc_dword(&i->count);
    return i;
}

void iput(struct inode *i)
{
    dec_dword(&i->count);
    /* TODO: write if dirty */
}

static unsigned int lookup_inode_block(struct inode *i, unsigned int blk)
{
    if (blk > 6) {
        panic("files above 7k not implemented");
    }

    return i->zones[blk];
}

int iread(struct inode *i, void *buf, unsigned int offset, unsigned int length)
{
    struct buffer *b;
    unsigned int blk, blk_off, devblk, copylen, total = 0;
    dev_t dev;

    mutex_lock(&i->lock);
    if (offset >= i->size) {
        mutex_unlock(&i->lock);
        return 0;
    } else if (offset + length >= i->size) {
        length = i->size - offset;
    }

    while (total < length) {
        blk = (offset + total) / BLOCKSIZE;
        blk_off = (offset + total) % BLOCKSIZE;

        if (MODE_TYPE(i->mode) == IFREG || MODE_TYPE(i->mode) == IFDIR) {
            dev = i->dev;
            devblk = lookup_inode_block(i, blk);
        } else if (MODE_TYPE(i->mode) == IFBLK) {
            dev = i->zones[0];
            devblk = blk;
        } else {
            mutex_unlock(&i->lock);
            return -ENOSYS;
        }

        b = readblk(dev, devblk);
        if (!b) {
            mutex_unlock(&i->lock);
            return total > 0 ? total : -EIO; /* TODO: error from driver */
        }
        copylen = MIN(BLOCKSIZE - blk_off, MIN(length - total, BLOCKSIZE));
        memcpy(buf + total, b->data + blk_off, copylen);

        relbuf(b);
        total += copylen;
    }

    mutex_unlock(&i->lock);
    return total;
}

int iwrite(struct inode *i, void *buf, unsigned int offset, unsigned int length)
{
    mutex_lock(&i->lock);
    if (MODE_TYPE(i->mode) == IFBLK) {
        if (offset >= i->size) {
            mutex_unlock(&i->lock);
            return 0;
        } else if (offset + length >= i->size) {
            length = i->size - offset;
        }
    }

    /* TODO: write to other things */
    mutex_unlock(&i->lock);
    return -ENOSYS;
}

static int scandir(struct inode **ip, struct inode *dir, char *name, int len)
{
    struct dentry de;
    unsigned int offset, i;
    int ret;

    for (offset = 0; offset < dir->size; offset += DENTRY_SIZE) {
        if (iread(dir, &de, offset, DENTRY_SIZE) < DENTRY_SIZE) {
            iput(dir);
            return -EIO;
        }

        for (i = 0; i < 14; i++) {
            if (de.name[i] == '\0')
                break;
        }
        if (i != len)
            continue;

        for (i = 0; i < len; i++) {
            if (name[i] != de.name[i])
                break;
        }
        if (i != len)
            continue;

        ret = iget(ip, dir->super, de.ino);
        iput(dir);
        return ret;
    }

    return -ENOENT;
}

int ilookup(struct inode **ip, char *path)
{
    /* TODO: follow mount points */
    struct inode *i;
    int len, ret;

    if (*path == '\0')
        return -ENOENT;
    else if (*path == '/')
        i = g_root_dir;
    else
        i = proc->cwd;

    idup(i);
    for (;;) {
        while (*path == '/')
            path++;
        if (*path == '\0') {
            *ip = i;
            return 0;
        }

        len = 0;
        while (path[len] != '\0' && path[len] != '/')
            len++;
        if (len > 14) {
            iput(i);
            return -ENOENT;
        }

        ret = scandir(&i, i, path, len);
        if (ret < 0)
            return ret;

        path += len;
        if (*path == '/' && MODE_TYPE(i->mode) != IFDIR) {
            iput(i);
            return -ENOTDIR;
        }
    }
}
