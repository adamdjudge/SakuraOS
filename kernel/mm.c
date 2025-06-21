/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#include <kernel.h>
#include <mm.h>
#include <x86.h>
#include <exception.h>
#include <fs.h>
#include <sched.h>
#include <signal.h>

/* Defined in linker script */
extern uint8_t _kernel_base[];
extern uint8_t _kernel_rw[];
extern uint8_t _kernel_end[];

uint32_t __attribute__((aligned(4096))) init_pdir[1024];
uint32_t __attribute__((aligned(4096))) init_ptab[1024];
uint32_t __attribute__((aligned(4096))) pagestack_ptab[1024];

/* Since page directory entry 1 references the page directory itself, virtual
 * addresses 0x400000 - 0x7fffff map to an array of all page table entries for
 * the entire virtual address space of the current context, pointed to by ptabs.
 * pdir maps to the currently loaded page directory. */
static uint32_t *ptabs = (uint32_t *)0x400000;
static uint32_t *pdir = (uint32_t *)0x401000;

/* Stack of free physical pages */
static uint32_t *pagestack = (uint32_t *)0x800000;
static unsigned int ps_size = 0;
static unsigned int stackp = 0;
static spinlock_t ps_lock;

/* Page reference counts for copy-on-write */
static uint16_t *pagecount;
static unsigned int pc_size;
static spinlock_t pc_lock;

static unsigned int npages;
static uint32_t kvaddr = 0xc00000;

extern void enable_paging();

static uint32_t pop_page()
{
    uint32_t page;

    spin_lock(&ps_lock);
    page = stackp > 0 ? pagestack[--stackp] : 0;
    if (page)
        npages++;
    spin_unlock(&ps_lock);
    return page;
}

static void push_page(uint32_t addr)
{
    spin_lock(&ps_lock);
    if (stackp < ps_size / sizeof(uint32_t)) {
        pagestack[stackp++] = addr;
        npages--;
    }
    spin_unlock(&ps_lock);
}

void mm_init()
{
    uint32_t addr, himem, i, ps_top;
    struct memrange *mr;

    printk("Initializing virtual memory manager\n");
    printk("  _kernel_base=0x%x\n  _kernel_rw=0x%x\n  _kernel_end=0x%x\n",
           (uint32_t)_kernel_base, (uint32_t)_kernel_rw, (uint32_t)_kernel_end);

    for (addr = (uint32_t)_kernel_base;
         addr < (uint32_t)_kernel_end;
         addr += PAGE_SIZE)
    {
        if (addr < (uint32_t)_kernel_rw)
            init_ptab[TABENT(addr)] = addr | PAGE_PRESENT;
        else
            init_ptab[TABENT(addr)] = addr | PAGE_PRESENT | PAGE_WRITABLE;
    }

    npages = (PAGE_ALIGN((uint32_t)_kernel_end) - (uint32_t)_kernel_base)
             / PAGE_SIZE;

    himem = 0;
    for (mr = g_memory_table; mr->size != 0; mr++) {
        if (mr->base >= HIMEM_BASE && mr->type == MEMTYPE_FREE)
            himem += mr->size;
    }

    /* Allocate a page to the page stack for each 4 mb of himem. The stack
       itself is located at the bottom of himem. */
    addr = HIMEM_BASE;
    for (i = 0; i < himem / (PAGE_SIZE * 1024) + 1; i++) {
        pagestack_ptab[i] = addr | PAGE_PRESENT | PAGE_WRITABLE;
        addr += PAGE_SIZE;
        ps_size += PAGE_SIZE;
        npages++;
    }
    ps_top = addr;

    /* Map the initial page table, initial page directory, and the page stack
     * page table into the address space. */
    init_pdir[0] = (uint32_t)init_ptab | PAGE_PRESENT | PAGE_WRITABLE;
    init_pdir[1] = (uint32_t)init_pdir | PAGE_PRESENT | PAGE_WRITABLE;
    init_pdir[2] = (uint32_t)pagestack_ptab | PAGE_PRESENT | PAGE_WRITABLE;
    
    /* Map VGA text memory area. */
    init_ptab[TABENT(0xb8000)] = 0xb8000 | PAGE_PRESENT | PAGE_WRITABLE;

    enable_paging();

    /* Push all pages of usable himem (that aren't part of the page stack)
       onto the page stack. */
    for (mr = g_memory_table; mr->size != 0; mr++) {
        if (mr->base < HIMEM_BASE || mr->type != MEMTYPE_FREE)
            continue;
        for (addr = mr->base; addr < mr->base + mr->size; addr += PAGE_SIZE) {
            if (addr >= ps_top)
                pagestack[stackp++] = addr;
        }
    }

    /* Allocate reference counts for copy-on-write pages. */
    pagecount = (uint16_t *)alloc_kernel_page(PAGE_WRITABLE);
    for (i = 0; i < himem / (PAGE_SIZE * 2048); i++)
        alloc_kernel_page(PAGE_WRITABLE);
    pc_size = (i+1) * PAGE_SIZE;
    memset(pagecount, 0, pc_size);
}

unsigned int mem_used()
{
    return npages * PAGE_SIZE;
}

bool map_page(uint32_t vaddr, uint32_t paddr, int flags)
{
    uint32_t pagetab;

    if ((pdir[DIRENT(vaddr)] & PAGE_PRESENT) == 0) {
        pagetab = pop_page();
        if (!pagetab)
            return false;
        pdir[DIRENT(vaddr)] = pagetab | PAGE_PRESENT | PAGE_WRITABLE | flags;
        flush_tlb();
        memset(&ptabs[DIRENT(vaddr)*1024], 0, PAGE_SIZE);
    }

    ptabs[TABENT(vaddr)] = paddr | PAGE_PRESENT | flags;
    return true;
}

bool alloc_page(uint32_t vaddr, int flags)
{
    uint32_t paddr;

    if ((paddr = pop_page()) == 0)
        return false;

    return map_page(vaddr, paddr, flags);
}

uint32_t alloc_kernel_page(int flags)
{
    // TODO: use an actual map of free page slots within the kernel's address
    // space so this never overflows into user address space
    
    uint32_t paddr;

    if ((paddr = pop_page()) == 0)
        return 0;
    
    if (map_page(kvaddr, paddr, flags)) {
        kvaddr += PAGE_SIZE;
        return kvaddr - PAGE_SIZE;
    }
    return 0;
}

void bump_kvaddr()
{
    kvaddr += PAGE_SIZE;
}

void free_page(uint32_t vaddr)
{
    uint32_t paddr = ptabs[TABENT(vaddr)] & ~PAGE_MASK;
    ptabs[TABENT(vaddr)] = 0;
    push_page(paddr);
}

uint32_t vtophys(uint32_t vaddr)
{
    return (ptabs[TABENT(vaddr)] & ~PAGE_MASK) + (vaddr & PAGE_MASK);
}

uint32_t check_page(uint32_t vaddr)
{
    if (!(pdir[DIRENT(vaddr)] & PAGE_PRESENT)) {
        return 0;
    }
    return ptabs[TABENT(vaddr)] & PAGE_MASK;
}

void set_page_writable(uint32_t vaddr, bool writable)
{
    if (!(pdir[DIRENT(vaddr)] & PAGE_PRESENT))
        return;

    if (writable)
        ptabs[TABENT(vaddr)] |= PAGE_WRITABLE;
    else
        ptabs[TABENT(vaddr)] &= ~PAGE_WRITABLE;
}

bool mm_add_mapping(uint32_t base, uint32_t size, uint32_t flags,
                    uint32_t file_offset, uint32_t file_size,
                    struct inode *inode)
{
    struct vmap *vm;

    spin_lock(&proc->mm_lock);

    for (vm = proc->vmaps; vm < proc->vmaps + NVMAPS; vm++) {
        if (vm->size == 0) {
            vm->base = base;
            vm->size = size;
            vm->flags = flags;
            vm->file_offset = file_offset;
            vm->file_size = file_size;
            vm->inode = inode;

            printk("mm: pid %d: vmap 0x%x-0x%x %s %s%s\n", proc->pid, base,
                   base + size - 1,
                   (flags & VMAP_WRITABLE) ? "writable" : "readonly",
                   (flags & VMAP_STACK) ? "stack " : "",
                   (flags & VMAP_SHARED) ? "shared " : "");
            break;
        }
    }

    spin_unlock(&proc->mm_lock);
    return vm != proc->vmaps + NVMAPS;
}

void mm_free_proc_memory()
{
    struct vmap *vm;
    uint32_t addr, i;

    for (vm = proc->vmaps; vm < proc->vmaps + NVMAPS; vm++) {
        for (addr = vm->base; addr < vm->base + vm->size; addr += PAGE_SIZE) {
            if (!check_page(addr))
                continue;

            spin_lock(&pc_lock);
            if (PAGECOUNT(addr) == 0) {
                spin_unlock(&pc_lock);
                free_page(addr);
            } else {
                PAGECOUNT(addr)--;
                spin_unlock(&pc_lock);
            }
        }

        vm->size = 0;
    }

    for (i = 256; i < 1024; i++) {
        if (pdir[i] & PAGE_PRESENT) {
            push_page(pdir[i] & ~PAGE_MASK);
            pdir[i] = 0;
        }
    }

    flush_tlb();
}

bool mm_fork_memory(uint32_t *new_pdir)
{
    struct vmap *vm;
    uint32_t addr, i, flags;

    spin_lock(&proc->mm_lock);

    /* Set each private writable page to copy-on-write and increment the
     * physical page reference count of all present pages. */
    for (vm = proc->vmaps; vm < proc->vmaps + NVMAPS; vm++) {
        for (addr = vm->base; addr < vm->base + vm->size; addr += PAGE_SIZE) {
            flags = check_page(addr);
            if (!(vm->flags & VMAP_SHARED) && (flags & PAGE_WRITABLE)) {
                ptabs[TABENT(addr)] &= ~PAGE_WRITABLE;
                ptabs[TABENT(addr)] |= PAGE_COPYONWRITE;
            }
            if (flags & PAGE_PRESENT)
                inc_word(&PAGECOUNT(addr));
        }
    }

    /* Copy all user page tables into the new process. Each physical page used
     * must be temporarily mapped into our own address space at the top page so
     * it can be written to, then it's added to the new page directory. */
    for (i = 256; i < 1024; i++) {
        if (pdir[i] & PAGE_PRESENT) {
            if (!alloc_page(0xfffff000, PAGE_WRITABLE)) {
                spin_unlock(&proc->mm_lock);
                panic("mm_fork_memory: out of memory"); // FIXME: un-cow pages
                return false;
            }
            flush_tlb();
            memcpy((void *)0xfffff000, ptabs + i*1024, PAGE_SIZE);
            new_pdir[i] = vtophys(0xfffff000)
                          | PAGE_PRESENT | PAGE_USER | PAGE_WRITABLE;
        }
    }

    flush_tlb();
    spin_unlock(&proc->mm_lock);
    return true;
}

static void pf_error(struct exception *e)
{
    if (e->err & PF_USER) {
        printk("warning: pid %d segmentation fault [%s 0x%x at 0x%x]\n",
               proc->pid, e->err & PF_WRITE ? "write" : "read", e->cr2, e->eip);
        thread->signal |= (1 << SIGSEGV);
    } else {
        dump_exception(e);
        panic("unexpected kmode page fault");
    }
}

static void pf_copy_on_write(uint32_t page)
{
    spin_lock(&pc_lock);

    if (PAGECOUNT(page) == 0) {
        spin_unlock(&pc_lock);
        ptabs[TABENT(page)] &= ~PAGE_COPYONWRITE;
        ptabs[TABENT(page)] |= PAGE_WRITABLE;
    } else {
        PAGECOUNT(page)--;
        spin_unlock(&pc_lock);

        /* Allocate new page temporarily mapped to the top of VM, copy contents
         * to it, then remap over the original page as writable. */
        if (!alloc_page(0xfffff000, PAGE_WRITABLE))
            panic("pf_copy_on_write: out of memory"); // FIXME
        flush_tlb();
        memcpy((void *)0xfffff000, (void *)page, PAGE_SIZE);
        ptabs[TABENT(page)] = vtophys(0xfffff000)
                              | PAGE_PRESENT | PAGE_USER | PAGE_WRITABLE;
    }

    flush_tlb();
}

static void pf_load_page(uint32_t page, struct vmap *vm)
{
    unsigned int offset, readlen, zerolen;
    int ret;

    offset = page - vm->base;
    if (offset < vm->file_size)
        readlen = MIN(PAGE_SIZE, vm->file_size - offset);
    else
        readlen = 0;
    zerolen = PAGE_SIZE - readlen;

    if (!alloc_page(page, PAGE_USER | PAGE_WRITABLE))
        panic("pf_load_page: out of memory"); // FIXME
    if (readlen > 0) {
        ret = iread(vm->inode, (void *)page, vm->file_offset + offset, readlen);
        if (ret < readlen)
            panic("pf_load_page: read failed"); // FIXME: segfault?
    }
    if (zerolen > 0)
        memset((void *)(page + readlen), 0, zerolen);

    if ((vm->flags & VMAP_WRITABLE) == 0) {
        set_page_writable(page, false);
        flush_tlb();
    }
    if (vm->flags & VMAP_STACK) {
        // TODO: check for collision with other mapping first
        vm->base -= PAGE_SIZE;
        vm->size += PAGE_SIZE;
    }
}

void handle_page_fault(struct exception *e)
{
    uint32_t page;
    struct vmap *vm;

    if (!proc) {
        dump_exception(e);
        panic("page fault during memory initialization");
    }
    ENABLE_INTERRUPTS;

    printk("page fault: pid %d: %s 0x%x\n", proc->pid,
           e->err & PF_WRITE ? "write" : "read", e->cr2);

    spin_lock(&proc->mm_lock);
    page = PAGE_BASE(e->cr2);
    for (vm = proc->vmaps; vm < proc->vmaps + NVMAPS; vm++) {
        if (page >= vm->base && page < vm->base + vm->size)
            break;
    }
    if (vm == proc->vmaps + NVMAPS) {
        pf_error(e);
        spin_unlock(&proc->mm_lock);
        return;
    }

    if ((e->err & PF_WRITE) && !(vm->flags & VMAP_WRITABLE))
        pf_error(e);
    else if ((e->err & PF_WRITE) && (check_page(page) & PAGE_COPYONWRITE))
        pf_copy_on_write(page);
    else if ((e->err & PF_PRESENT) == 0)
        pf_load_page(page, vm);
    else
        pf_error(e);

    spin_unlock(&proc->mm_lock);
}
