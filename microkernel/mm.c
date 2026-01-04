/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#include "kernel.h"
#include "exception.h"
#include "mm.h"

/* Defined in linker script */
extern char __kernel_base[];
extern char __kernel_rw[];
extern char __kernel_end[];

uint32_t __attribute__((aligned(PAGE_SIZE))) kpagetable[64*1024];
uint32_t __attribute__((aligned(PAGE_SIZE))) kpagedir[1024];

void mm_init()
{
    debug("initializing virtual memory: base=0x%x rw=0x%x end=0x%x",
          (uint32_t)__kernel_base, (uint32_t)__kernel_rw,
          (uint32_t)__kernel_end);
    
    /* Identity-map the bottom 128M of physical memory, including the kernel,
     * but write-protect the kernel text segment. */
    for (uint32_t addr = 0; addr < 128*1024*1024; addr += PAGE_SIZE) {
        if (addr >= (uint32_t)__kernel_base && addr < (uint32_t)__kernel_end)
            kpagetable[addr / PAGE_SIZE] = addr | PAGE_PRESENT;
        else
            kpagetable[addr / PAGE_SIZE] = addr | PAGE_PRESENT | PAGE_WRITABLE;
    }
    for (int i = 0; i < 32; i++)
        kpagedir[i] = (uint32_t)&kpagetable[i*1024]
                      | PAGE_PRESENT | PAGE_WRITABLE;
    
    /* Enable virtual address translation. */
    __asm__ __volatile__(
        "mov $kpagedir, %%eax;"
        "mov %%eax, %%cr3;"
        "mov %%cr0, %%eax;"
        "or $0x80010000, %%eax;"
        "mov %%eax, %%cr0;"
        ::: "memory"
    );
}
