/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#include "kernel.h"

extern char cpuid_vendor[];
extern char cpuid_brand[];
extern uint16_t cpuid_base_freq;

void kmain()
{
    init_debug();
    debug("starting SakuraOS kernel...");
    debug("processor: %s %s @ %d MHz", cpuid_vendor, cpuid_brand,
          cpuid_base_freq);
}
