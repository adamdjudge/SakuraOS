/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: buffer.h
 */

#ifndef BUFFER_H
#define BUFFER_H

#define BUF_BLOCKSIZE 1024
#define NUM_BUFFERS 64

#define BUF_LOCK 0x01
#define BUF_UPTODATE 0x02
#define BUF_DIRTY 0x04

struct buffer {
    int flags;
    dev_t dev;
    int block;
    unsigned int time;
    char data[BUF_BLOCKSIZE];
};

struct buffer *getbuf(dev_t dev, int block);
void relbuf(struct buffer *b);

#endif
