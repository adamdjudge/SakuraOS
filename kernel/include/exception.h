/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#ifndef EXCEPTION_H
#define EXCEPTION_H

/* i386 exception numbers */
enum {
    ENO_DIVISION_BY_ZERO = 0,
    ENO_DEBUG = 1,
    ENO_NMI = 2,
    ENO_BREAKPOINT = 3,
    ENO_INTO_DETECTED_OVERFLOW = 4,
    ENO_OUT_OF_BOUNDS = 5,
    ENO_INVALID_OPCODE = 6,
    ENO_NO_COPROCESSOR = 7,
    ENO_DOUBLE_FAULT = 8,
    ENO_COPROCESSOR_SEGMENT_OVERRUN = 9,
    ENO_BAD_TSS = 10,
    ENO_SEGMENT_NOT_PRESENT = 11,
    ENO_STACK_FAULT = 12,
    ENO_GENERAL_PROTECTION_FAULT = 13,
    ENO_PAGE_FAULT = 14,
    ENO_RESERVED = 15,
    ENO_COPROCESSOR_FAULT = 16,
    ENO_ALIGNMENT_CHECK = 17,
    ENO_MACHINE_CHECK = 18,
    ENO_SIMD_FP = 19,

    ENO_IRQ0 = 32,
    ENO_IRQ1 = 33,
    ENO_IRQ2 = 34,
    ENO_IRQ3 = 35,
    ENO_IRQ4 = 36,
    ENO_IRQ5 = 37,
    ENO_IRQ6 = 38,
    ENO_IRQ7 = 39,
    ENO_IRQ8 = 40,
    ENO_IRQ9 = 41,
    ENO_IRQ10 = 42,
    ENO_IRQ11 = 43,
    ENO_IRQ12 = 44,
    ENO_IRQ13 = 45,
    ENO_IRQ14 = 46,
    ENO_IRQ15 = 47,

    ENO_SYSCALL = 255
};

/**
 * Struct pushed onto the stack when a processor exception or hardware interrupt
 * occurs. Contains the state of the task that was interrupted.
 */
struct exception {
    uint32_t cr0;
    uint32_t cr2;
    uint32_t cr3;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t k_esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;
    uint32_t eno; /* Exception number */
    uint32_t err; /* Optional error code */
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;

    /* Only valid when coming from user task. */
    uint32_t esp;
    uint32_t ss;
};

#define KERNEL_EXCEPTION(e)  (e.cs == 0x8)
#define USER_EXCEPTION(e)    (e.cs != 0x8)

#define PF_PRESENT 0x1
#define PF_WRITE 0x2
#define PF_USER 0x4

void dump_exception(struct exception *e);

#endif
