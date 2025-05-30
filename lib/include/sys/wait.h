/**
 * The SakuraOS Standard Library
 * Copyright 2025 Adam Judge
 */

#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#define WNOHANG 1
#define WUNTRACED 2

#define WIFEXITED(wstatus)    ((wstatus & 0x100) == 0)
#define WEXITSTATUS(wstatus)  (wstatus & 0xff)
#define WIFSIGNALED(wstatus)  ((wstatus & 0x100) != 0)
#define WTERMSIG(wstatus)     (wstatus & 0xff)

pid_t waitpid(pid_t, int *, int);

#define wait(wstatus) waitpid(-1, wstatus, 0)

#endif
