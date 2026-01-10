/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#include "kernel.h"
#include "mm.h"

extern char cpuid_vendor[];
extern char cpuid_brand[];
extern uint16_t cpuid_base_freq;

struct kip __attribute__((aligned(PAGE_SIZE))) kip;

extern void debug_init();
extern void mm_init(struct kpb *kpb);

void kmain(struct kpb *kpb)
{
    debug_init();
    debug("starting SakuraOS kernel...");
    debug("compiled %s %s", __DATE__, __TIME__);
    debug("processor: %s %s @ %d MHz", cpuid_vendor, cpuid_brand,
          cpuid_base_freq);

    for (int i = 0; i < MAX_MODULES; i++)
        kip.modules[i] = kpb->modules[i];
    mm_init(kpb);
}
