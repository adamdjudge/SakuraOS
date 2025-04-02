/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#ifndef X86_H
#define X86_H

#define DISABLE_INTERRUPTS __asm__("cli")
#define ENABLE_INTERRUPTS __asm__("sti")
#define BREAKPOINT __asm__("int3")

#define MEMTYPE_FREE 1

struct memrange {
    uint32_t base;
    uint32_t _resvd1;
    uint32_t size;
    uint32_t _resvd2;
    uint32_t type;
};

extern struct memrange g_memory_table[];

extern char g_cpuid_vendor[];
extern char g_cpuid_brand[];
extern uint16_t g_cpuid_base_freq;

extern void out_byte(uint16_t port, uint8_t data);
extern void out_byte_wait(uint16_t port, uint8_t data);
extern uint8_t in_byte(uint16_t port);
extern uint8_t in_byte_wait(uint16_t port);

#endif
