/**
 * The SakuraOS Standard Library
 * Copyright 2025 Adam Judge
 */

#ifndef _UNISTD_H
#define _UNISTD_H

#ifndef NULL
#define NULL (void *)0
#endif

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

unsigned int alarm(unsigned int);
int close(int);
int execve(const char *, char *const [], char *const []);
void _exit(int);
pid_t fork(void);
ssize_t read(int, void *, size_t);

/* Temporary debug syscall */
void print(char *);

#endif
