/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#include "kernel.h"
#include "exception.h"

extern void handle_timer_irq();

void handle_exception(struct exception e)
{
    switch (e.eno) {
    case ENO_PAGE_FAULT:
        uint32_t cr2;
        __asm__ __volatile__("mov %%cr2, %0" : "=r"(cr2));
        debug("kernel panic: unexpected page fault 0x%x at 0x%x", cr2, e.eip);
        for (;;) {}
        break;
    case ENO_IRQ0:
        handle_timer_irq();
        break;
    default:
        if (KERNEL_EXCEPTION(e)) {
            debug("kernel panic: unexpected exception %d", e.eno);
            for (;;) {}
        }
    }
}
