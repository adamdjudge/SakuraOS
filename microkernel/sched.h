/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#ifndef SCHED_H
#define SCHED_H

#define NUM_TASKS (128*K)

enum {
    TASK_NONE,
    TASK_RUN,
    TASK_BLOCK,
};

 struct task {
    uint32_t pid;
    uint32_t state;
    uint32_t esp;
    uint32_t cr3;
    uint32_t pfhandler;
    uint32_t exhandler;
    struct task *prev;
    struct task *next;
    char stack[1024 - 8*4];
};

extern struct task *current;

int sched_create_task(uint32_t addrspace_pid, uint32_t pfhandler,
                      uint32_t exhandler);

#endif
