/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#ifndef MM_H
#define MM_H

#define PAGE_SIZE 4096
#define PAGE_MASK 0xfff

#define K 1024
#define M K*K
#define G M*K

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



#endif
