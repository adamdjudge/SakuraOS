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

struct task *current;
//static struct task *runlist_head, *runlist_tail;
//static struct task *blocklist_head, *blocklist_tail;
struct task idle;

//static unsigned int next_task;
static unsigned int rem_quantum;

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
