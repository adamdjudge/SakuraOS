/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#ifndef MM_H
#define MM_H

#include <fs.h>

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

/**
 * Physical page reference count.
 */
#define PAGECOUNT(vaddr)  pagecount[(vtophys(vaddr)-HIMEM_BASE) >> 12]

/**
 * Memory mapping within a process's virtual address space.
 */
struct vmap {
    uint32_t base;
    uint32_t size;
    uint32_t flags;
    uint32_t file_offset;
    uint32_t file_size;
    struct inode *inode;
};

/**
 * Memory mapping flags.
 */
#define VMAP_READONLY  0
#define VMAP_WRITABLE  (1<<0)
#define VMAP_STACK     (1<<1)
#define VMAP_SHARED    (1<<2)

/**
 * Number of memory mappings per process.
 */
#define NVMAPS 8

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
bool mm_add_mapping(uint32_t base, uint32_t size, uint32_t flags,
                    uint32_t file_offset, uint32_t file_size,
                    struct inode *inode);
void mm_free_proc_memory();
bool mm_fork_memory(uint32_t *new_pdir);

#endif
