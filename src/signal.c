/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: signal.c
 */

#include <kernel.h>
#include <x86.h>
#include <exception.h>
#include <fs.h>
#include <sched.h>
#include <signal.h>

uint32_t signal_pending()
{
    if (thread->proc)
        return (proc->signal | thread->signal) & ~thread->sigmask;
    else
        return thread->signal & ~thread->sigmask;
}

void send_proc_signal(struct proc *p, int signum)
{
    p->signal |= (1 << signum);
    if (p->state == PS_STOPPED && signum == SIGCONT)
        p->state = PS_RUNNING;

    /* If all threads in interruptible sleep, wake one to process signal */
    sched_interrupt_proc(p);
}

void handle_signal(struct exception *e)
{
    int signum = 0;
    struct sigframe *frame;

    /* There is no standard zeroth signal, so bit 0 of the thread bitfield is
     * used to immediately terminate the thread. */
    if (thread->signal & 0x1) {
        if (thread->proc)
            dec_dword(&proc->nthreads);
        thread->state = TS_NONE;
        yield_thread();
    }

    while ((signal_pending() & (1 << signum)) == 0)
        signum++;

    thread->signal &= ~(1 << signum);
    proc->signal &= ~(1 << signum);

    ENABLE_INTERRUPTS;

    if (signum == SIGKILL) {
        printk("pid %d terminated by SIGKILL\n", proc->pid);
        sched_terminate(SIGKILL << 8);
    } else if (signum == SIGSTOP) {
        printk("pid %d stopped by SIGSTOP\n", proc->pid);
        block_thread_interruptible(); // huh?
    }

    if (proc->sigdisp[signum] == SIG_IGN) {
        return;
    } else if (proc->sigdisp[signum] == SIG_DFL) {
        switch (signum) {
        case SIGABRT:
        case SIGBUS:
        case SIGFPE:
        case SIGILL:
        case SIGQUIT:
        case SIGSEGV:
        case SIGSYS:
        case SIGTRAP:
        case SIGXCPU:
        case SIGXFSZ:
        /* TODO: the above signals should core dump, not just terminate */

        case SIGALRM:
        case SIGHUP:
        case SIGINT:
        case SIGIO:
        case SIGPIPE:
        case SIGPROF:
        case SIGPWR:
        case SIGSTKFLT:
        case SIGTERM:
        case SIGUSR1:
        case SIGUSR2:
        case SIGVTALRM:
            printk("pid %d terminated by signal %d\n", proc->pid, signum);
            sched_terminate(signum << 8);

        case SIGTSTP:
        case SIGTTIN:
        case SIGTTOU:
            printk("pid %d stopped by signal %d\n", proc->pid, signum);
            block_thread_interruptible();
            return;
        }
    }

    /* Setup signal handler stack frame */
    e->esp -= sizeof(struct sigframe);
    frame = (struct sigframe *)e->esp;
    frame->trampoline[0] = 0xb8; // mov eax, -1
    frame->trampoline[1] = 0xff;
    frame->trampoline[2] = 0xff;
    frame->trampoline[3] = 0xff;
    frame->trampoline[4] = 0xff;
    frame->trampoline[5] = 0xcd; // int 255
    frame->trampoline[6] = 0xff;
    frame->trampoline[7] = 0x00; // pad
    frame->retaddr = (uint32_t)&frame->trampoline;
    frame->eax = e->eax;
    frame->ebx = e->ebx;
    frame->ecx = e->ecx;
    frame->edx = e->edx;
    frame->esi = e->esi;
    frame->edi = e->edi;
    frame->ebp = e->ebp;
    frame->eip = e->eip;
    frame->eflags = e->eflags;
    frame->signum = signum;

    e->eip = proc->sigdisp[signum];
    proc->sigdisp[signum] = SIG_DFL;
}

void sys_sigreturn(struct exception *e)
{
    struct sigframe *frame;

    e->esp -= sizeof(uint32_t); // because retaddr was popped
    frame = (struct sigframe *)e->esp;
    e->esp += sizeof(struct sigframe);
    e->eax = frame->eax;
    e->ebx = frame->ebx;
    e->ecx = frame->ecx;
    e->edx = frame->edx;
    e->esi = frame->esi;
    e->edi = frame->edi;
    e->ebp = frame->ebp;
    e->eip = frame->eip;
    e->eflags = frame->eflags;
}
