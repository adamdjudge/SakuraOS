/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#include <kernel.h>
#include <x86.h>
#include <exception.h>
#include <fs.h>
#include <mm.h>
#include <sched.h>
#include <signal.h>

/* Programmable Interrupt Timer Registers */
#define PIT_CMD  0x43
#define PIT_DATA 0x40

/* Assembly routines */
extern void switch_context();
extern void iret_from_exception();

extern uint32_t init_pdir[];

static struct proc procs[NPROCS];
struct proc *proc;

static struct thread threads[NTHREADS];
static struct thread *idle;
struct thread *thread;
struct thread *next_thread;

static unsigned int next_pid;
static unsigned int schedule_timer;
static unsigned int njiffies;
static spinlock_t sched_lock;

void sched_init()
{
    struct proc *p;
    struct thread *t;

    printk("Starting scheduler\n");

    thread = idle = threads;
    next_pid = 1;
    schedule_timer = SCHED_FREQ;
    idle->state = TS_RUNNING;
    idle->counter = -1;

    for (p = procs; p < procs + NPROCS; p++) {
        p->pdir = (uint32_t *)alloc_kernel_page(PAGE_WRITABLE);
        if (!p->pdir)
            panic("failed to allocate process page directory");
        p->cr3 = vtophys((uint32_t)p->pdir);
    }

    for (t = threads; t < threads + NTHREADS; t++) {
        bump_kvaddr(); /* Create gap to catch overflow */
        t->kstack = (void *)alloc_kernel_page(PAGE_WRITABLE);
        if (!t->kstack)
            panic("failed to allocate thread kernel stack");
        t->tss_esp0 = (uint32_t)t->kstack + PAGE_SIZE;
    }

    /* Program the timer. */
    out_byte_wait(PIT_CMD, 0x36);
    out_byte_wait(PIT_DATA, TIMER_DIVIDER & 0xff);
    out_byte_wait(PIT_DATA, (TIMER_DIVIDER >> 8) & 0xff);

    ENABLE_INTERRUPTS;
}

void handle_timer_irq()
{
    struct proc *p;
    struct thread *t;

    njiffies++;

    for (p = procs; p < procs + NPROCS; p++) {
        p->rtime += 10;
        if (p->alarm && --p->alarm == 0)
            p->signal |= (1 << SIGALRM);
    }

    for (t = threads; t < threads + NTHREADS; t++) {
        if (t->state == TS_INTERRUPTIBLE) {
            if (t->sleep != 0 && t->sleep <= njiffies) {
                t->sleep = 0;
                t->state = TS_RUNNING;
                t->counter = 0x7fffffff;
                schedule();
                return;
            }
        }
    }

    if (--schedule_timer == 0) {
        schedule();
    }
}

void schedule()
{
    struct thread *t;

    schedule_timer = SCHED_FREQ;

    next_thread = idle;
    for (t = threads; t < threads + NTHREADS; t++) {
        if (t->proc && t->proc->state != PS_RUNNING)
            continue;
        if (t->state == TS_RUNNING && t->counter > next_thread->counter)
            next_thread = t;
    }

    for (t = threads; t < threads + NTHREADS; t++) {
        if (t != idle) {
            t->counter++;
        }
    }

    if (next_thread != idle) {
        next_thread->counter = 0;
    }

    switch_context();
}

struct proc *get_process(int pid)
{
    struct proc *p;

    for (p = procs; p < procs + NPROCS; p++) {
        if (p->state != PS_NONE && p->pid == pid)
            return p;
    }

    return NULL;
}

void yield_thread()
{
    DISABLE_INTERRUPTS;
    schedule();
    ENABLE_INTERRUPTS;
}

void block_thread_interruptible()
{
    DISABLE_INTERRUPTS;
    thread->state = TS_INTERRUPTIBLE;
    schedule();
    ENABLE_INTERRUPTS;
}

void block_thread_uninterruptible()
{
    DISABLE_INTERRUPTS;
    thread->state = TS_UNINTERRUPTIBLE;
    schedule();
    ENABLE_INTERRUPTS;
}


void sleep_thread(unsigned int time)
{
    DISABLE_INTERRUPTS;
    thread->sleep = njiffies + time;
    thread->state = TS_INTERRUPTIBLE;
    schedule();
    ENABLE_INTERRUPTS;
}

unsigned int jiffies()
{
    return njiffies;
}

struct proc *create_proc()
{
    struct proc *p;

    spin_lock(&sched_lock);

    for (p = procs; p < procs + NPROCS; p++) {
        if (p->state == PS_NONE)
            break;
    }
    if (p == procs + NPROCS) {
        spin_unlock(&sched_lock);
        return NULL;
    }

    p->state = PS_RUNNING;
    spin_unlock(&sched_lock);

    p->pid = next_pid++;
    p->alarm = 0;
    p->rtime = 0;
    p->ktime = 0;
    p->utime = 0;
    p->next_tid = 1;
    p->nthreads = 0;
    p->exit_status = 0;
    p->signal = 0;
    p->mm_lock = 0;

    memcpy(p->pdir, init_pdir, PAGE_SIZE);
    p->pdir[1] = p->cr3 | PAGE_PRESENT | PAGE_WRITABLE;

    return p;
}

struct thread *create_thread(struct proc *proc)
{
    struct thread *t;

    spin_lock(&sched_lock);

    for (t = threads; t < threads + NTHREADS; t++) {
        if (t->state == TS_NONE)
            break;
    }
    if (t == threads + NTHREADS) {
        spin_unlock(&sched_lock);
        return NULL;
    }

    t->state = TS_INTERRUPTIBLE;
    spin_unlock(&sched_lock);

    t->counter = 0;
    t->sleep = 0;
    t->signal = 0;
    t->sigmask = 0;
    t->proc = proc;
    t->tid = proc ? proc->next_tid++ : 0;
    if (proc)
        inc_dword(&proc->nthreads);

    t->esp = (uint32_t)t->kstack + PAGE_SIZE;
    memset(t->kstack, 0, PAGE_SIZE);

    return t;
}

void sched_stop_thread()
{
    if (thread->proc)
        dec_dword(&proc->nthreads);
    thread->state = TS_NONE;
    yield_thread();
}

void sched_stop_other_threads()
{
    static spinlock_t lock = 0;
    struct thread *t;

    spin_lock(&lock);

    /* In case another thread is terminating the process at the same time and
     * already stopped us while we were waiting on the lock. */
    if (thread->signal & SIG_KILL_THREAD) {
        spin_unlock(&lock);
        sched_stop_thread();
    }

    for (t = threads; t < threads + NTHREADS; t++) {
        if (t->proc && t->proc == proc && t != thread) {
            t->signal |= 0x1;
            if (t->state == TS_INTERRUPTIBLE)
                t->state = TS_RUNNING;
        }
    }

    spin_unlock(&lock);

    while (proc->nthreads > 1)
        yield_thread();
}

void sched_terminate(int exit_status)
{
    struct proc *p, *pp;
    int i;

    printk("pid %d exiting with status 0x%x\n", proc->pid, exit_status);

    if (proc->pid == 1)
        panic("tried to kill init");

    sched_stop_other_threads();

    for (p = procs; p < procs + NPROCS; p++) {
        if (p->ppid == proc->pid && p->state != PS_NONE) {
            p->ppid = 1;
            if (p->state == PS_ZOMBIE)
                send_proc_signal(get_process(1), SIGCHLD);
        }
    }

    pp = get_process(p->ppid);
    if (pp)
        send_proc_signal(pp, SIGCHLD);

    mm_free_proc_memory();
    iput(proc->exe);
    iput(proc->cwd);
    for (i = 0; i < OPEN_MAX; i++) {
        if (proc->files[i])
            close(i);
    }

    proc->exit_status = exit_status;
    proc->state = PS_ZOMBIE;
    thread->state = TS_NONE;
    yield_thread();
}

int sched_waitpid(int pid, int *wstatus, int options)
{
    struct proc *p;
    bool found = false;

    if (options & ~0x3)
        return -EINVAL;

    for (;;) {
        spin_lock(&sched_lock);
        for (p = procs; p < procs + NPROCS; p++) {
            if (p->state == PS_NONE || p->ppid != proc->pid)
                continue;
            else if ((pid < -1 && p->pgid == -pid)
                     || pid == -1
                     || (pid == 0 && p->pgid == proc->pgid)
                     || p->pid == pid)
            {
                found = true;
                if (p->state == PS_ZOMBIE)
                    goto done;
            }
        }
        spin_unlock(&sched_lock);

        if (!found)
            return -ECHILD;
        else if (options & 0x1) /* WNOHANG */
            return 0;

        yield_thread();
        if (signal_pending())
            return -EINTR;
    }

done:
    spin_unlock(&sched_lock);
    if (wstatus)
        *wstatus = p->exit_status;
    pid = p->pid;
    p->state = PS_NONE;
    return pid;
}

void sched_interrupt_proc(struct proc *proc)
{
    /* TODO: be careful of uninterruptible threads */
    struct thread *t;

    for (t = threads; t < threads + NTHREADS; t++) {
        if (t->proc && t->proc->pid == proc->pid
            && t->state != TS_NONE && t->state != TS_INTERRUPTIBLE)
        {
            return;
        }
    }

    for (t = threads; t < threads + NTHREADS; t++) {
        if (t->proc && t->proc->pid == proc->pid)
            t->state = TS_RUNNING;
    }
}
