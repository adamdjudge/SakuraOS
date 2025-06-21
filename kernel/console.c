/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#include <kernel.h>
#include <console.h>
#include <x86.h>

static char color;
static int pos;

static char *textmem = (char *) 0xb8000;

static spinlock_t console_lock;

static void update_cursor()
{
    out_byte(0x3D4, 0x0F); // Index cursor location low register
    out_byte(0x3D5, pos & 0xFF);
    out_byte(0x3D4, 0x0E); // Index cursor location high register
    out_byte(0x3D5, (pos >> 8) & 0xFF);
}

void console_init()
{
    int i;

    color = BG_BLACK | TEXT_WHITE;
    pos = 0;

    for (i = 0; i < SCREEN_NCHARS; i++) {
        textmem[2*i] = '\0';
        textmem[2*i+1] = BG_BLACK | TEXT_WHITE;
    }

    /* Set cursor to block. */
    out_byte(0x3D4, 0x0A);                         // Index cursor start register
    out_byte(0x3D5, (in_byte(0x3D5) & 0xC0));      // Start scanline = 0
    out_byte(0x3D4, 0x0B);                         // Index cursor end register
    out_byte(0x3D5, (in_byte(0x3D5) & 0xE0) | 15); // End scanline = 15

    update_cursor();
}

static void linefeed()
{
    int i;

    for (i = 1; i < SCREEN_HEIGHT; i++) {
        memcpy(&textmem[(i-1) * SCREEN_WIDTH * 2],
               &textmem[i * SCREEN_WIDTH * 2],
               2 * SCREEN_WIDTH);
    }

    for (i = 0; i < SCREEN_WIDTH; i++) {
        textmem[2*(SCREEN_NCHARS-SCREEN_WIDTH+i)] = '\0';
        textmem[2*(SCREEN_NCHARS-SCREEN_WIDTH+i)+1] = BG_BLACK | TEXT_WHITE;
    }
}

void console_putc(char c)
{
    spin_lock(&console_lock);

    switch (c) {
    case '\n':
        do {
            textmem[2*pos] = ' ';
            textmem[2*pos+1] = color;
            pos++;
        } while (pos % SCREEN_WIDTH != 0);
        break;
    case '\b':
        if (pos > 0) {
            pos--;
            textmem[2*pos] = ' ';
            textmem[2*pos+1] = color;
        }
        break;
    default:
        textmem[2*pos] = c;
        textmem[2*pos+1] = color;
        pos++;
    }

    if (pos >= SCREEN_NCHARS) {
        pos = SCREEN_NCHARS - SCREEN_WIDTH;
        linefeed();
    }

    update_cursor();
    spin_unlock(&console_lock);
}
