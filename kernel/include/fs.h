/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: fs.h
 */

#ifndef FS_H
#define FS_H

typedef unsigned short dev_t;
#define MAJOR(dev) (dev >> 8)
#define MINOR(dev) (dev & 0xff)

#define BLOCKSIZE 1024

#define MINIX_14_MAGIC 0x137f
#define MINIX_30_MAGIC 0x138f

struct superblock {
    unsigned short ninodes;
    unsigned short nzones;
    unsigned short imap_blocks;
    unsigned short zmap_blocks;
    unsigned short first_data_zone;
    unsigned short log_zone_size;
    unsigned int max_size;
    unsigned short magic;
    unsigned short state;

    /* Not stored on disk */
    dev_t dev;
};

#define SB_SIZE 20
#define NUM_SUPERS 8

struct inode {
    unsigned short mode;
    unsigned short uid;
    unsigned int size;
    unsigned int time;
    unsigned char gid;
    unsigned char nlinks;
    unsigned short zones[9];

    /* Not stored on disk */
    mutex_t lock;
    dev_t dev;
    unsigned int inum;
    unsigned int count;
    struct superblock *super;
    struct inode *mount;
};

#define INODE_SIZE 32
#define INODES_PER_BLOCK 32
#define NUM_INODES 64

#define INODE_ERROR 0x01
#define INODE_DIRTY 0x02

/* Inode mode flags */
#define MODE_TYPE(m) (m & 0170000)
#define IFSOCK  0140000  /* Socket */
#define IFLNK   0120000  /* Symbolic link */
#define IFREG   0100000  /* Regular file */
#define IFBLK   0060000  /* Block device */
#define IFDIR   0040000  /* Directory */
#define IFCHR   0020000  /* Character device */
#define IFIFO   0010000  /* Pipe */
#define ISUID   0004000  /* Set user ID */
#define ISGID   0002000  /* Set group ID */
#define IFVTX   0001000  /* Sticky bit */
#define IRUSR   0000400  /* User readable */
#define IWUSR   0000200  /* User writable */
#define IXUSR   0000100  /* User executable */
#define IRGRP   0000040  /* Group readable */
#define IWGRP   0000020  /* Group writable */
#define IXGRP   0000010  /* Group executable */
#define IROTH   0000004  /* Other readable */
#define IWOTH   0000002  /* Other writable */
#define IXOTH   0000001  /* Other executable */

/* Root directory of filesystem tree */
extern struct inode *g_root_dir;

struct dentry {
    unsigned short ino;
    char name[14];
};

#define DENTRY_SIZE 16

struct file {
    unsigned int flags;
    unsigned int pos;
    unsigned int count;
    struct inode *inode;
    mutex_t lock;
};

#define NUM_FILES 64
#define OPEN_MAX 16

enum {
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
    O_INVALID_ACCMODE
};

#define O_ACCMODE  0x3
#define O_APPEND   (1<<2)
#define O_CLOEXEC  (1<<3)
#define O_CREAT    (1<<4)
#define O_DSYNC    (1<<5)
#define O_EXCL     (1<<6)
#define O_NOCTTY   (1<<7)
#define O_NONBLOCK (1<<8)
#define O_SYNC     (1<<9)
#define O_TRUNC    (1<<10)

#define RW_MAX 0x7ffff000

int mount(dev_t dev, struct inode **ip);
int iget(struct inode **ip, struct superblock *s, unsigned int inum);
struct inode *idup(struct inode *i);
void iput(struct inode *i);
int iread(struct inode *i, void *buf, unsigned int offset, unsigned int length);
int ilookup(struct inode **ip, char *path);

int open(char *path, unsigned int flags, unsigned int creat_mode);
int close(int fd);
int read(int fd, char *buf, unsigned int length);

#endif
