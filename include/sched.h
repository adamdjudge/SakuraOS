/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#ifndef SCHED_H
#define SCHED_H

/**
 * Divider frequency for the PIT chip, which should cause an IRQ 0 interrupt
 * every ~10 ms.
 */
#define TIMER_DIVIDER 11932

/**
 * Number of timer interrupts for thread scheduling quantum.
 */
#define SCHED_FREQ 10

/**
 * Maximum number of processes that can exist at once.
 */
#define NPROCS 64

/**
 * Maximum number of threads that can exist at once (including the idle thread).
 */
#define NTHREADS 128

struct exe_header {
    unsigned int magic;
    unsigned int flags;
    unsigned int text_size;
    unsigned int data_size;
};

/**
 * Process state values.
 */
enum {
    PS_NONE,
    PS_RUNNING,
    PS_STOPPED,
    PS_ZOMBIE
};

/**
 * Process state struct.
 */
struct proc {
    /* Process memory management */
    uint32_t *pdir;        /* Page directory */
    uint32_t cr3;          /* Page directory physical address */
    mutex_t mm_lock;       /* Lock for memory management */

    unsigned int state;    /* Process state */
    unsigned int pid;      /* Process ID */
    unsigned int ppid;     /* Parent process ID */
    unsigned int pgid;     /* Process group ID */
    unsigned int sid;      /* Session ID */
    unsigned int uid;      /* User ID */
    unsigned int gid;      /* Group ID */
    unsigned int euid;     /* Effective user ID */
    unsigned int egid;     /* Effective group ID */
    unsigned int alarm;    /* Alarm clock set by task */
    unsigned int rtime;    /* Real time elapsed in millis */
    unsigned int ktime;    /* Kernel time elapsed in millis */
    unsigned int utime;    /* User time elapsed in millis */
    unsigned int next_tid; /* Next thread ID */
    unsigned int nthreads; /* Number of threads */
    unsigned int signal;   /* Signal bit field */
    int exit_status;       /* Exit status for waitpid */

    struct inode *cwd;     /* Current working directory */

    uint32_t text_base;
    uint32_t data_base;
    uint32_t data_top;
    uint32_t stack_base;
    struct inode *exe;
    struct exe_header hdr;

    uint32_t sigdisp[32];
};

/**
 * Thread state values.
 */
enum {
    TS_NONE,
    TS_RUNNING,
    TS_INTERRUPTIBLE,
    TS_UNINTERRUPTIBLE
};

/**
 * Thread state struct.
 */
struct thread {
    /* Used for context switching */
    uint32_t esp;         /* Saved kernel ESP */
    uint32_t tss_esp0;    /* Kernel ESP switched to by exception */

    struct proc *proc;    /* Owning process */
    void *kstack;         /* Kernel stack page */
    unsigned int tid;     /* Process thread ID */
    int state;            /* Thread state */
    int counter;          /* schedule() calls since last run */
    unsigned int sleep;   /* Remaining sleep time */
    unsigned int signal;  /* Signal bit field */
    unsigned int sigmask; /* Signal mask */
};

/**
 * The currently running process and thread.
 */
extern struct proc *proc;
extern struct thread *thread;

void sched_init();
void schedule();
struct proc *get_process(int pid);
void yield_thread();
void block_thread_interruptible();
void block_thread_uninterruptible();
void sleep_thread(unsigned int time);
unsigned int jiffies();
struct proc *create_proc();
struct thread *create_thread(struct proc *proc);
void sched_stop_thread();
void sched_stop_other_threads();
void sched_terminate(int exit_status);
int sched_waitpid(int pid, int *wstatus, int options);
void sched_interrupt_proc(struct proc *proc);

#endif
