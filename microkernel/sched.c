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

static unsigned int next_pid;
static unsigned int rem_quantum;
static unsigned int active_tasks;

extern uint32_t tss_esp0;
extern void iret_from_exception();

void sched_init()
{
    debug("initializing scheduler");

    current = &idle;
    next_pid = 1;
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

/*
 * Kernel stack template for newly created tasks. Contains register contents and
 * dummy ebp popped on context switch, the address of iret_from_exception to
 * return to, and an exception with userspace register contents.
 */
struct init_kstack {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t _esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t dummy_ebp; // Popped by "leave" instruction
    uint32_t ret_addr;
    struct exception exception;
};

/*
 * Instantiates a new TCB entry and returns its PID, or a negative error code.
 * Sets the new task's page fault handler and general exception handler to
 * pfhandler and exhandler. If addrspace_pid is a valid PID, the task is created
 * as a thread within the same address space; otherwise the task is given a new
 * empty address space.
 */
int sched_create_task(uint32_t addrspace_pid, uint32_t pfhandler,
                      uint32_t exhandler)
{
    if (active_tasks == NUM_TASKS)
        return -ERR_NO_FREE_TCB;

    struct task *t;
    do {
        t = TASKS + (next_pid % NUM_TASKS);
        if (!mm_is_page_writable((uint32_t)t)) {
            if (!mm_alloc_page((uint32_t)t, PAGE_WRITABLE))
                return -ERR_OUT_OF_MEMORY;
        }
    } while (t->state != TASK_NONE);

    if (addrspace_pid) {
        struct task *ast = TASKS + (addrspace_pid % NUM_TASKS);
        if (ast->state == TASK_NONE)
            return -ERR_BAD_PID;
        t->cr3 = ast->cr3;
    } else {
        t->cr3 = mm_create_addr_space();
        if (!t->cr3)
            return -ERR_OUT_OF_MEMORY;
    }

    t->esp = (uint32_t)(t + 1) - sizeof(struct init_kstack);
    struct init_kstack *kstack = (struct init_kstack *)t->esp;
    kstack->ret_addr = (uint32_t)iret_from_exception;
    kstack->ebp = t->esp + 32; // Make "leave" instruction happy

    t->pid = next_pid++;
    t->pfhandler = pfhandler;
    t->exhandler = exhandler;
    active_tasks++;
    return t->pid;
}
