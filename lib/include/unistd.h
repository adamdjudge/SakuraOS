/**
 * The SakuraOS Standard Library
 * Copyright 2025 Adam Judge
 */

#ifndef _UNISTD_H
#define _UNISTD_H

#define NULL (void *)0

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

unsigned int alarm(unsigned int);
int execve(const char *, char *const [], char *const []);
void _exit(int);
pid_t fork(void);

/* Temporary debug syscall */
void print(char *);

#endif
