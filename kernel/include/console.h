/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#define SCREEN_WIDTH    80
#define SCREEN_HEIGHT   25
#define SCREEN_NCHARS   SCREEN_WIDTH * SCREEN_HEIGHT
#define SCREEN_TABSTOP  4

/* Text colors */
enum {
    TEXT_BLACK,
    TEXT_BLUE,
    TEXT_GREEN,
    TEXT_CYAN,
    TEXT_RED,
    TEXT_MAGENTA,
    TEXT_BROWN,
    TEXT_LIGHTGRAY,
    TEXT_GRAY,
    TEXT_LIGHTBLUE,
    TEXT_LIGHTGREEN,
    TEXT_LIGHTCYAN,
    TEXT_LIGHTRED,
    TEXT_LIGHTMAGENTA,
    TEXT_YELLOW,
    TEXT_WHITE,

    TEXT_BLINK = 0x80,
};

/* Background colors */
enum {
    BG_BLACK      = 0x00,
    BG_BLUE       = 0x10,
    BG_GREEN      = 0x20,
    BG_CYAN       = 0x30,
    BG_RED        = 0x40,
    BG_MAGENTA    = 0x50,
    BG_BROWN      = 0x60,
    BG_LIGHTGRAY  = 0x70,
};

void console_init();
void console_putc(char c);

#endif
