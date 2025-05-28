/**
 * The SakuraOS Standard Library
 * Copyright 2025 Adam Judge
 */

#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#define WNOHANG 1
#define WUNTRACED 2

pid_t waitpid(pid_t, int *, int);

#define wait(wstatus) waitpid(-1, wstatus, 0)

#endif
