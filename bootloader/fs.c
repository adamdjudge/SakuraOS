/**
 * The SakuraOS Bootloader
 * Copyright 2026 Adam Judge
 */

#include "bootloader.h"
#include "fs.h"
#include "string.h"
#include "floppy.h"

char tmpblk[BLOCKSIZE];
unsigned short curblk;
unsigned short inodeblk;
struct dentry bootdir_dentries[DENTRIES_PER_BLOCK];

/* Loads the "/boot" directory contents from disk. As a side effect, inodeblk
 * is set to the first block of the inode array. */
bool fs_init()
{
    if (!floppy_read_block(tmpblk, 0, 1)) {
        printf("fs: failed to read superblock\n");
        return false;
    }

    struct superblock super;
    memcpy(&super, tmpblk, SB_SIZE);
    if (super.magic != MINIX_14_MAGIC) {
        printf("fs: invalid file system\n");
        return false;
    }

    inodeblk = 2 + super.imap_blocks + super.zmap_blocks;
    if (!floppy_read_block(tmpblk, 0, inodeblk))
        return false;

    struct inode rootdir;
    memcpy(&rootdir, tmpblk, INODE_SIZE);
    if (!floppy_read_block(tmpblk, 0, rootdir.zones[0]))
        return false;

    struct dentry *root_dentries = (struct dentry *)tmpblk;
    unsigned short boot_ino = 0;
    for (int i = 0; i < DENTRIES_PER_BLOCK; i++) {
        if (strcmp(root_dentries[i].name, "boot")) {
            boot_ino = root_dentries[i].ino;
            break;
        }
    }
    if (boot_ino == 0)
        return false;

    boot_ino -= 1;
    if (!floppy_read_block(tmpblk, 0, inodeblk + (boot_ino / INODES_PER_BLOCK)))
        return false;
    
    struct inode bootdir;
    memcpy(&bootdir, tmpblk + INODE_SIZE * (boot_ino % INODES_PER_BLOCK),
           INODE_SIZE);
    if (!floppy_read_block(bootdir_dentries, 0, bootdir.zones[0]))
        return false;

    curblk = 0;
    return true;
}

/* Looks up an inode within the "/boot" directory. */
bool get_bootdir_inode(struct inode *inode, char *name)
{
    unsigned short ino = 0;
    for (int i = 0; i < DENTRIES_PER_BLOCK; i++) {
        if (strcmp(bootdir_dentries[i].name, name)) {
            ino = bootdir_dentries[i].ino;
            break;
        }
    }
    if (ino == 0)
        return false;

    ino -= 1;
    curblk = 0;
    if (!floppy_read_block(tmpblk, 0, inodeblk + (ino / INODES_PER_BLOCK)))
        return false;
    memcpy(inode, tmpblk + INODE_SIZE * (ino % INODES_PER_BLOCK), INODE_SIZE);
    return true;
}

bool fileread(struct inode *inode, void *buffer, unsigned int offset,
              unsigned int length)
{
    if (offset + length > inode->size)
        return false;

    static unsigned short indirect_block[BLOCKSIZE / 2];
    static unsigned short last_indirect_block = 0;
    if (inode->zones[7] && last_indirect_block != inode->zones[7]) {
        last_indirect_block = inode->zones[7];
        if (!floppy_read_block(indirect_block, 0, inode->zones[7]))
            return false;
    }

    unsigned short file_block = offset / BLOCKSIZE;
    while (length) {
        offset %= BLOCKSIZE;
        unsigned short block = file_block > 6 ? indirect_block[file_block - 7]
                                              : inode->zones[file_block];
        file_block++;
        if (block != curblk) {
            curblk = block;
            if (!floppy_read_block(tmpblk, 0, block))
                return false;
        }

        unsigned int copylen = MIN(length, BLOCKSIZE - offset);
        memcpy(buffer, tmpblk + offset, copylen);
        buffer += copylen;
        offset += copylen;
        length -= copylen;
    }

    return true;
}
