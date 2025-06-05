/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#include <kernel.h>
#include <exception.h>
#include <sched.h>
#include <console.h>
#include <x86.h>
#include <mm.h>
#include <fs.h>
#include <signal.h>

extern void handle_timer_irq();
extern void handle_keyboard_irq();
extern void handle_floppy_irq();
extern void floppy_update_timer();

extern void handle_page_fault(struct exception *e);
extern void syscall(struct exception *e);
extern void handle_signal(struct exception *e);

void dump_exception(struct exception *e)
{
    printk("%s Exception %d [%x] EIP=%x\n", e->cs == 0x8 ? "Kernel" : "User",
           e->eno, e->err, e->eip);
    printk("  EAX=%x EBX=%x ECX=%x EDX=%x\n", e->eax, e->ebx, e->ecx, e->edx);
    printk("  ESI=%x EDI=%x EBP=%x ESP=%x\n", e->esi, e->edi, e->ebp,
           e->cs == 0x8 ? e->k_esp : e->esp);
    printk("  EFL=%x CR0=%x CR2=%x CR3=%x\n", e->eflags, e->cr0, e->cr2, e->cr3);
}

void handle_exception(struct exception e)
{
    switch (e.eno) {
    case ENO_IRQ0:
        handle_timer_irq();
        floppy_update_timer();
        break;
    case ENO_IRQ1:
        handle_keyboard_irq();
        break;
    case ENO_IRQ6:
        handle_floppy_irq();
        break;
    case ENO_IRQ12:
        // TODO: PS/2 mouse interrupt
        break;
    case ENO_DIVISION_BY_ZERO:
        if (KERNEL_EXCEPTION(e)) {
            dump_exception(&e);
            panic("division by zero");
        } else {
            thread->signal |= (1 << SIGFPE);
        }
        break;
    case ENO_BREAKPOINT:
        if (KERNEL_EXCEPTION(e)) {
            printk("*** KERNEL BREAKPOINT ***\n");
            dump_exception(&e);
            printk("--- END BREAKPOINT ---\n");
        } else {
            printk("*** USER BREAKPOINT ***\n");
            dump_exception(&e);
            printk("--- END BREAKPOINT ---\n");
        }
        break;
    case ENO_INVALID_OPCODE:
        if (KERNEL_EXCEPTION(e)) {
            dump_exception(&e);
            panic("invalid opcode");
        } else {
            thread->signal |= (1 << SIGILL);
        }
        break;
    case ENO_GENERAL_PROTECTION_FAULT:
        if (KERNEL_EXCEPTION(e)) {
            dump_exception(&e);
            panic("general protection fault");
        } else {
            thread->signal |= (1 << SIGILL);
        }
        break;
    case ENO_PAGE_FAULT:
        handle_page_fault(&e);
        break;
    case ENO_SYSCALL:
        syscall(&e);
        break;
    default:
        dump_exception(&e);
        panic("unhandled exception");
    }

    DISABLE_INTERRUPTS;
    if (USER_EXCEPTION(e) && signal_pending())
        handle_signal(&e);
}
