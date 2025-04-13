/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: syscall.c
 */

#include <kernel.h>
#include <fs.h>
#include <exception.h>
#include <mm.h>
#include <sched.h>
#include <x86.h>
#include <signal.h>

void sys_exit(struct exception *e)
{
    printk("pid %d exited with status %d\n", proc->pid, e->ebx & 0xff);
    sched_terminate(e->ebx & 0xff);
}

int sys_waitpid(struct exception *e)
{
    return sched_waitpid(e->ebx, (int *)e->ecx, e->edx);
}

int sys_alarm(struct exception *e)
{
    int ret = proc->alarm / 100;
    if (!e->ebx)
        return ret;

    proc->alarm = e->ebx * 100;
    return ret;
}

static bool can_kill(struct proc *p, int signum)
{
    if (p->pid == 1 && p->sigdisp[signum] <= SIG_IGN)
        return false;
    else if (proc->uid == 0)
        return true;
    else if (signum == SIGCONT && p->sid == proc->sid)
        return true;
    else
        return p->uid == proc->uid;
}

int sys_kill(struct exception *e)
{
    int pid = e->ebx, signum = e->ecx;
    struct proc *p;

    /* TODO: handle other cases */
    if (pid < 1)
        return -ESRCH;
    if (signum < 0 || signum > SIGSYS)
        return -EINVAL;

    p = get_process(pid);
    if (!p)
        return -ESRCH;
    if (!can_kill(p, signum))
        return -EPERM;

    if (signum != 0)
        send_proc_signal(p, signum);
    return 0;
}

int sys_signal(struct exception *e)
{
    if (e->ebx == 0 || e->ebx > SIGSYS)
        return -EINVAL;

    proc->sigdisp[e->ebx] = e->ecx;
    return 0;
}

int sys_execve(struct exception *e)
{
    int ret, i;
    struct inode *exe;
    struct exe_header hdr;
    uint32_t vaddr;
    char *filename = (char *)e->ebx;

    ret = ilookup(&exe, filename);
    if (ret) {
        printk("execve: could not open '%s'\n", filename);
        return ret;
    }

    /* TODO: permission check */

    ret = iread(exe, (char *)&hdr, 0, sizeof(hdr));
    if (ret < 0) {
        iput(exe);
        printk("execve: failed to read %s\n", filename);
        return ret;
    } else if (ret < sizeof(hdr) || hdr.magic != 0xCAFEBABE) {
        iput(exe);
        printk("execve: bad file %s\n", filename);
        return -ENOEXEC;
    }

    if (proc->exe)
        iput(proc->exe);

    printk("execve: %s [flags 0x%x, text %d, data %d]\n",
           filename, hdr.flags, hdr.text_size, hdr.data_size);
    
    sched_stop_other_threads();
    mm_free_proc_memory();

    proc->exe = exe;
    mm_add_mapping(
        0x80000000,
        hdr.text_size,
        VMAP_READONLY,
        sizeof(hdr),
        hdr.text_size,
        exe
    );
    mm_add_mapping(
        0x80000000 + PAGE_ALIGN(hdr.text_size),
        hdr.data_size,
        VMAP_WRITABLE,
        sizeof(hdr) + hdr.text_size,
        exe->size - sizeof(hdr) - hdr.text_size,
        exe
    );
    mm_add_mapping(
        0xffffe000,
        PAGE_SIZE,
        VMAP_WRITABLE | VMAP_STACK,
        0,
        0,
        NULL
    );

    proc->rtime = 0;
    proc->ktime = 0;
    proc->utime = 0;
    for (i = 0; i < 32; i++) {
        if (proc->sigdisp[i] > SIG_IGN)
            proc->sigdisp[i] = SIG_DFL;
    }
    thread->tid = 1;
    proc->next_tid = 2;

    e->cs = 0x1B;
    e->ds = 0x23;
    e->es = 0x23;
    e->fs = 0x23;
    e->gs = 0x23;
    e->ss = 0x23;
    e->eax = 0;
    e->ebx = 0;
    e->ecx = 0;
    e->edx = 0;
    e->edi = 0;
    e->esi = 0;
    e->ebp = 0;
    e->esp = 0xfffff000;
    e->eflags = (1 << 9);
    e->eip = 0x80000000;
    return 0;
}

struct init_kstack {
    uint32_t regs[8];
    uint32_t ret_addr;
    struct exception except;
};

extern void iret_from_exception();

int sys_fork(struct exception *e)
{
    struct proc *new_proc;
    struct thread *new_thread;
    struct init_kstack *kstack;

    new_proc = create_proc();
    if (!new_proc) {
        printk("WARNING: fork: process table full\n");
        return -EAGAIN;
    }

    /* TODO: we need to copy all of the calling process's threads... */
    new_thread = create_thread(new_proc);
    if (!new_thread) {
        printk("WARNING: fork: thread table full\n");
        new_proc->state = PS_NONE;
        return -EAGAIN;
    }

    if (!mm_fork_memory(new_proc->pdir)) {
        printk("WARNING: fork: out of memory\n");
        new_proc->state = PS_NONE;
        new_thread->state = TS_NONE;
        return -ENOMEM;
    }
    memcpy(new_proc->vmaps, proc->vmaps, sizeof(proc->vmaps));

    new_proc->ppid = proc->pid;
    new_proc->pgid = proc->pgid;
    new_proc->sid = proc->sid;
    new_proc->uid = proc->uid;
    new_proc->gid = proc->gid;
    new_proc->euid = proc->euid;
    new_proc->egid = proc->egid;
    new_proc->cwd = proc->cwd;
    new_proc->exe = proc->exe;

    idup(proc->cwd);
    idup(proc->exe);

    new_thread->esp -= sizeof(struct init_kstack);
    kstack = (struct init_kstack *)new_thread->esp;
    kstack->ret_addr = (uint32_t)iret_from_exception;
    kstack->except = *e;
    kstack->except.eax = 0;

    printk("fork: pid %d -> pid %d\n", proc->pid, new_proc->pid);
    new_thread->state = TS_RUNNING;
    return new_proc->pid;
}

void syscall(struct exception *e)
{
    ENABLE_INTERRUPTS;

    switch (e->eax) {
    case 0xffffffff:
        sys_sigreturn(e);
        break;
    case 0:
        sys_exit(e);
    case 1:
        e->eax = sys_waitpid(e);
        break;
    case 2:
        e->eax = sys_alarm(e);
        break;
    case 3:
        e->eax = sys_kill(e);
        break;
    case 4:
        e->eax = sys_signal(e);
        break;
    case 11:
        e->eax = sys_execve(e);
        break;
    case 12:
        e->eax = sys_fork(e);
        break;
    case 69:
        // TEMPORARY
        printk("%s", (char*)e->ebx);
        break;
    default:
        printk("pid %d tried invalid syscall %d\n", proc->pid, e->eax);
        e->eax = -ENOSYS;
    }
}
