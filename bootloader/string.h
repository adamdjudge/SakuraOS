/**
 * The SakuraOS Bootloader
 * Copyright 2025 Adam Judge
 */

#ifndef STRING_H
#define STRING_H

void memcpy(char *dest, char *src, unsigned int len);
void vsprintf(char *buf, const char *fmt, ...);
void printf(const char *fmt, ...);

#endif
