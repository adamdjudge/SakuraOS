/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#ifndef MM_H
#define MM_H

#define PAGE_SIZE 4096
#define PAGE_MASK 0xfff

#define HIMEM_BASE 0x100000

/**
 * Page attribute flags for Page Directory and Page Table entries.
 */
#define PAGE_PRESENT      (1<<0)
#define PAGE_WRITABLE     (1<<1)
#define PAGE_USER         (1<<2)
#define PAGE_COPYONWRITE  (1<<9)

/**
 * Round a size up to the nearest page size.
 */
#define PAGE_ALIGN(n)  ((n + PAGE_MASK) & ~PAGE_MASK)

/**
 * Round an address down to its page base.
 */
#define PAGE_BASE(a)  (a & ~PAGE_MASK)

/**
 * Get the Page Directory index or Page Table index of a virtual address.
 */
#define DIRENT(v)  (v >> 22)
#define TABENT(v)  (v >> 12)

void mm_init();
extern void flush_tlb();
unsigned int mem_used();
bool map_page(uint32_t vaddr, uint32_t paddr, int flags);
bool alloc_page(uint32_t vaddr, int flags);
uint32_t alloc_kernel_page(int flags);
void bump_kvaddr();
void free_page(uint32_t vaddr);
uint32_t vtophys(uint32_t vaddr);
uint32_t check_page(uint32_t vaddr);
void set_page_writable(uint32_t vaddr, bool writable);
void free_proc_memory();
bool fork_memory(struct proc *newproc);

#endif
