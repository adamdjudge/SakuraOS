/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#include <kernel.h>
#include <exception.h>
#include <fs.h>
#include <sched.h>

struct init_kstack {
    uint32_t regs[8];
    uint32_t ret_addr;
};

extern void floppy_init();
extern void execve(char *filename, char **argv, char **envp);

static void do_init()
{
    printk("Started init process\n");

    floppy_init();

    mount(0x200, &g_root_dir);
    proc->cwd = g_root_dir;

    execve("/bin/init", NULL, NULL);
    execve("/init", NULL, NULL);
    panic("failed to start init");
}

void create_init()
{
    struct proc *init_proc;
    struct thread *init_thread;
    struct init_kstack *kstack;

    init_proc = create_proc();
    if (!init_proc)
        panic("failed to create init process");

    init_proc->ppid = 0;
    init_proc->pgid = 0;
    init_proc->sid = 0;
    init_proc->uid = 0;
    init_proc->gid = 0;
    init_proc->euid = 0;
    init_proc->egid = 0;
    init_proc->text_base = 0;

    init_thread = create_thread(init_proc);
    if (!init_thread)
        panic("failed to create init thread");

    init_thread->esp -= sizeof(struct init_kstack);
    kstack = (struct init_kstack *)init_thread->esp;
    kstack->ret_addr = (uint32_t)do_init;

    init_thread->state = TS_RUNNING;
}
