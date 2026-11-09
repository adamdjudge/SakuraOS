/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#include "kernel.h"

extern char cpuid_vendor[];
extern char cpuid_brand[];
extern uint16_t cpuid_base_freq;

extern void uart_init(unsigned int baudrate);

void kmain()
{
    uart_init(9600);
    debug("Starting SakuraOS kernel...");
    debug("Compiled %s %s", __DATE__, __TIME__);
    debug("Processor: %s %s @ %d MHz", cpuid_vendor, cpuid_brand,
          cpuid_base_freq);
}
