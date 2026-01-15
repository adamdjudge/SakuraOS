/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#include "kernel.h"
#include "exception.h"
#include "mm.h"
#include "sched.h"

#define PIT_CMD 0x43
#define PIT_DATA 0x40
#define PIT_DIVIDER 1193

#define QUANTUM_MS 20

#define TASKS ((struct task *)(256*M))

struct task *current, *runlist;
//static struct task *blocklist;
struct task __attribute__((aligned(sizeof(struct task)))) idle;

//static unsigned int next_task;
static unsigned int rem_quantum;

extern uint32_t tss_esp0;

void sched_init()
{
    debug("initializing scheduler");

    current = &idle;
    rem_quantum = QUANTUM_MS;

    out_byte_wait(PIT_CMD, 0x36);
    out_byte_wait(PIT_DATA, PIT_DIVIDER & 0xff);
    out_byte_wait(PIT_DATA, (PIT_DIVIDER >> 8) & 0xff);
}

void schedule()
{
    struct task *selected;
    if (runlist) {
        selected = runlist;
        runlist = runlist->next;
    } else {
        selected = &idle;
    }

    /* Switch to new virtual address space if it's different. */
    if (selected != &idle && selected->cr3 != current->cr3)
        __asm__ __volatile__("mov %0, %%cr3" :: "r"(selected->cr3));

    /* Set kernel stack vector to the top of the TCB. */
    tss_esp0 = (uint32_t)(selected + 1);

    /* Perform context switch. */
    current = selected;
    __asm__ __volatile__(
        "pusha;"
        "mov %%esp, %%edi;"
        "and $0xfffffc00, %%edi;"
        "mov current, %%esi;"
        "mov %%esp, 8(%%edi);"
        "mov 8(%%esi), %%esp;"
        "popa;"
        ::: "memory"
    );
}

void handle_timer_irq()
{
    kip.uptime_us += 1000;
    if (kip.uptime_us == 1000000) {
        kip.uptime_us = 0;
        kip.uptime_s++;
    }

    if (--rem_quantum == 0) {
        rem_quantum = QUANTUM_MS;
        schedule();
    }
}
