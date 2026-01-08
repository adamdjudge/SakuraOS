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

/* Kernel page tables mapping the bottom 384M of virtual memory */
uint32_t __attribute__((aligned(PAGE_SIZE))) kpagetable[96*K];
uint32_t __attribute__((aligned(PAGE_SIZE))) kpagedir[K];

/* Page stack for physical pages under 256M */
uint32_t pagestack[64*K];
unsigned int pagesp;

static inline void push_frame(uint32_t paddr)
{
    pagestack[pagesp++] = paddr;
}

static inline uint32_t pop_frame()
{
    return pagesp ? pagestack[--pagesp] : 0;
}

void mm_init(struct kpb *kpb)
{
    debug("kernel text segment: 0x%x - 0x%x",
          (uint32_t)__kernel_base, (uint32_t)__kernel_rw - 1);
    debug("kernel data segment: 0x%x - 0x%x",
          (uint32_t)__kernel_rw, (uint32_t)__kernel_end - 1);
        
    unsigned int memtotal = 640*K;
    for (struct memrange *mr = kpb->memtab; mr < kpb->memtab + MAX_MEMRANGES; mr++) {
        if (mr->size == 0)
            break;
        else if (mr->type == 1)
            memtotal += mr->size;
        debug("mem 0x%x - 0x%x %s", mr->base, mr->base + mr->size - 1,
              mr->type == 1 ? "free" : "resvd");
    }
    debug("total usable memory: %d KB", memtotal / K);

    unsigned int kmem = PAGE_ALIGN(MIN(256*M, MAX(1*M, memtotal / 8)));
    debug("kernel reserved memory: %d KB", kmem / K);

    /* Find the end of where modules have been loaded by the bootloader. This
     * marks the start of free himem pages. */
    uint32_t modules_top = PAGE_ALIGN((uint32_t)__kernel_end);
    for (struct module *mod = kpb->modules; mod < kpb->modules + MAX_MODULES; mod++) {
        if (mod->start_addr == 0)
            break;
        debug("module: start_addr=0x%x", mod->start_addr);

        for (struct segment *seg = mod->segments; seg < mod->segments + MAX_SEGMENTS; seg++) {
            if (seg->size == 0)
                break;
            debug("  segment: paddr=0x%x vaddr=0x%x size=0x%x flags=0x%x",
                  seg->paddr, seg->vaddr, seg->size, seg->flags);

            uint32_t seg_top = PAGE_ALIGN(seg->paddr + seg->size);
            modules_top = MAX(modules_top, seg_top);
        }
    }

    unsigned int kmem_free =
        kmem - (PAGE_ALIGN((uint32_t)__kernel_end) - (uint32_t)__kernel_base);

    /* Push kernel reserved memory pages into the page stack. Everything above
     * that will later be granted to userspace. */
    uint32_t kmem_top = modules_top;
    for (uint32_t addr = PAGE_SIZE; addr < 640*K; addr += PAGE_SIZE)
        push_frame(addr);
    for (struct memrange *mr = kpb->memtab; mr < kpb->memtab + MAX_MEMRANGES; mr++) {
        if (mr->size == 0)
            break;
        else if (mr->type != 1)
            continue;
        for (uint32_t addr = mr->base; addr < mr->base + mr->size; addr += PAGE_SIZE) {
            if (pagesp * PAGE_SIZE >= kmem_free)
                break;
            if (addr >= modules_top) {
                push_frame(addr);
                kmem_top += PAGE_SIZE;
            }
        }
    }
    debug("kmem_top=0x%x", kmem_top);

    /* Map page tables into the page directory. */
    for (int i = 0; i < 96; i++) {
        uint32_t addr = (uint32_t)kpagetable + i * PAGE_SIZE;
        kpagedir[i] = addr | PAGE_PRESENT | PAGE_WRITABLE;
    }

    /* Identity-map the bottom 256M of physical memory, including the kernel,
     * but write-protect the kernel text segment. */
    for (uint32_t addr = PAGE_SIZE; addr < 256*M; addr += PAGE_SIZE) {
        if (addr >= (uint32_t)__kernel_base && addr < (uint32_t)__kernel_end)
            kpagetable[addr / PAGE_SIZE] = addr | PAGE_PRESENT;
        else
            kpagetable[addr / PAGE_SIZE] = addr | PAGE_PRESENT | PAGE_WRITABLE;
    }
    
    /* Enable virtual address translation. */
    __asm__ __volatile__(
        "mov $kpagedir, %%eax;"
        "mov %%eax, %%cr3;"
        "mov %%cr0, %%eax;"
        "or $0x80010000, %%eax;"
        "mov %%eax, %%cr0;"
        ::: "memory"
    );
    debug("virtual address translation enabled");
}
