/**
 * The SakuraOS Bootloader
 * Copyright 2026 Adam Judge
 */

#ifndef STRING_H
#define STRING_H

void memcpy(void *dest, void *src, unsigned int len);
bool strcmp(char *a, char *b);
void memset(void *s, char c, unsigned int n);
void vsprintf(char *buf, const char *fmt, ...);
void printf(const char *fmt, ...);

#endif
