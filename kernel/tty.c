/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#include <kernel.h>
#include <console.h>
#include <sched.h>
#include <signal.h>

#define NUM_TTYS 3

struct tty {
    mutex_t lock;
    struct thread *waiting;
    int flags;
    unsigned int head;
    unsigned int tail;
    char buffer[256];
};

struct tty ttys[NUM_TTYS];

extern void tty_handle_input(uint8_t minor, char c)
{
    struct tty *tty = &ttys[minor];

    if (c == '\b' && tty->head != tty->tail)
        tty->head = (tty->head-1) % sizeof(tty->buffer);
    else if (c != '\b' && (tty->head+1) % sizeof(tty->buffer) != tty->tail) {
        tty->buffer[tty->head++] = c;
        tty->head %= sizeof(tty->buffer);
    } else
        return;

    if (minor == 0)
        console_putc(c);
    if (c == '\n' && tty->waiting)
        tty->waiting->state = TS_RUNNING;
}

int tty_read(uint8_t minor, char *buf, unsigned int length)
{
    struct tty *tty;
    int count = 0;

    if (minor >= NUM_TTYS)
        return -ENODEV;
    tty = &ttys[minor];

    mutex_lock(&tty->lock);
    tty->waiting = thread;
    block_thread_interruptible();
    if (signal_pending())
        return -EINTR;

    while (count < length && tty->tail != tty->head) {
        *buf = tty->buffer[tty->tail++];
        tty->tail %= sizeof(tty->buffer);
        count++;
        if (*buf++ == '\n')
            break;
    }

    mutex_unlock(&tty->lock);
    return count;
}

int tty_write(uint8_t minor, char *buf, unsigned int length)
{
    int i, ret;

    if (minor >= NUM_TTYS)
        return -ENODEV;

    mutex_lock(&ttys[minor].lock);
    if (minor == 0) {
        for (i = 0; i < length; i++)
            console_putc(buf[i]);
        ret = length;
    } else {
        ret = -ENODEV;
    }
    mutex_unlock(&ttys[minor].lock);
    return ret;
}
