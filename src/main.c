/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#include <kernel.h>
#include <console.h>
#include <x86.h>
#include <sched.h>
#include <mm.h>

#include <serial.h>

extern void create_init();

void kmain()
{
    serial_init();
    console_init();
    printk("Starting SakuraOS...\n");
    printk("Processor: %s %s @ %d MHz\n",
           g_cpuid_vendor, g_cpuid_brand, g_cpuid_base_freq);
    
    printk("Memory map from BIOS:\n");
    for (struct memrange *mr = g_memory_table; mr->size != 0; mr++)
        printk("  %s 0x%x - 0x%x (%uk)\n",
               mr->type == 1 ? "free" : "resv", mr->base,
               mr->base + mr->size - 1, mr->size / 1024);
    
    mm_init();
    sched_init();
    create_init();
    printk("Memory used: %d kb\n", mem_used() / 1024);
}
