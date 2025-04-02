/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 * File: signal.h
 */

#ifndef SIGNAL_H
#define SIGNAL_H

/**
 * Signal values.
 */
enum {
    SIGHUP = 1,
    SIGINT,
    SIGQUIT,
    SIGILL,
    SIGTRAP,
    SIGABRT,
    SIGBUS,
    SIGFPE,
    SIGKILL,
    SIGUSR1,
    SIGSEGV,
    SIGUSR2,
    SIGPIPE,
    SIGALRM,
    SIGTERM,
    SIGSTKFLT,
    SIGCHLD,
    SIGCONT,
    SIGSTOP,
    SIGTSTP,
    SIGTTIN,
    SIGTTOU,
    SIGURG,
    SIGXCPU,
    SIGXFSZ,
    SIGVTALRM,
    SIGPROF,
    SIGWINCH,
    SIGIO,
    SIGPWR,
    SIGSYS
};

/**
 * Signal dispositions (if not a function pointer).
 */
#define SIG_DFL 0
#define SIG_IGN 1

/**
 * Signal handler stack frame.
 */
struct sigframe {
    uint32_t retaddr;
    uint32_t signum;
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t eip;
    uint32_t eflags;
    uint8_t trampoline[8];
};

uint32_t signal_pending();
void send_proc_signal(struct proc *p, int signum);
void sys_sigreturn(struct exception *e);

#endif
