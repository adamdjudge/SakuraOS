/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#include <kernel.h>
#include <x86.h>
#include <exception.h>
#include <fs.h>
#include <sched.h>
#include <mm.h>
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
static mutex_t ps_lock;

/* Page reference counts for copy-on-write */
static uint16_t *pagecount;
static unsigned int pc_size;
static mutex_t pc_lock;

static unsigned int npages;
static uint32_t kvaddr = 0xc00000;

extern void enable_paging();

static uint32_t pop_page()
{
    uint32_t page;

    mutex_lock(&ps_lock);
    page = stackp > 0 ? pagestack[--stackp] : 0;
    if (page)
        npages++;
    mutex_unlock(&ps_lock);
    return page;
}

static void push_page(uint32_t addr)
{
    mutex_lock(&ps_lock);
    if (stackp < ps_size / sizeof(uint32_t)) {
        pagestack[stackp++] = addr;
        npages--;
    }
    mutex_unlock(&ps_lock);
}

void mm_init()
{
    uint32_t signature, addr, himem, i, ps_top;
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
    ptabs[TABENT(vaddr)] &= ~PAGE_PRESENT;
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

static void free_proc_page(uint32_t addr)
{
    if (!check_page(addr))
        return;

    mutex_lock(&pc_lock);
    if (pagecount[(vtophys(addr)-HIMEM_BASE) >> 12] == 0) {
        mutex_unlock(&pc_lock);
        free_page(addr);
    } else {
        pagecount[(vtophys(addr)-HIMEM_BASE) >> 12]--;
        mutex_unlock(&pc_lock);
    }
}

void free_proc_memory()
{
    uint32_t addr, i;

    if (proc->text_base == 0)
        return;

    for (addr = proc->text_base; addr < proc->data_top; addr += PAGE_SIZE)
        free_proc_page(addr);
    for (addr = proc->stack_base; addr < 0xfffff000; addr += PAGE_SIZE)
        free_proc_page(addr);

    for (i = 512; i < 1024; i++) {
        if (pdir[i] & PAGE_PRESENT) {
            push_page(pdir[i] & ~PAGE_MASK);
            pdir[i] = 0;
        }
    }

    flush_tlb();
}

static void cowpage(uint32_t addr)
{
    uint32_t paddr = vtophys(addr);
    uint32_t flags = check_page(addr);

    if (flags & PAGE_WRITABLE) {
        ptabs[TABENT(addr)] &= ~PAGE_WRITABLE;
        ptabs[TABENT(addr)] |= PAGE_COPYONWRITE;
    }

    if (flags & PAGE_PRESENT)
        inc_word(&pagecount[(paddr-HIMEM_BASE) >> 12]);
}

bool fork_memory(struct proc *newproc)
{
    uint32_t addr, i;

    if (proc->text_base == 0)
        return true;

    mutex_lock(&proc->mm_lock);

    for (addr = proc->text_base; addr < proc->data_top; addr += PAGE_SIZE)
        cowpage(addr);
    for (addr = proc->stack_base; addr < 0xfffff000; addr += PAGE_SIZE)
        cowpage(addr);

    for (i = 512; i < 1024; i++) {
        if (pdir[i] & PAGE_PRESENT) {
            if (!alloc_page(0xfffff000, PAGE_WRITABLE))
                goto nomem;
            flush_tlb();
            memcpy((void *)0xfffff000, &ptabs[i*1024], PAGE_SIZE);
            newproc->pdir[i] = vtophys(0xfffff000)
                               | PAGE_PRESENT | PAGE_USER | PAGE_WRITABLE;
        }
    }

    flush_tlb();
    mutex_unlock(&proc->mm_lock);
    return true;

nomem:
    for (i = 512; i < 1024; i++) {
        if (newproc->pdir[i] & PAGE_PRESENT) {
            push_page(newproc->pdir[i] & ~PAGE_MASK);
            newproc->pdir[i] = 0;
        }
    }
    flush_tlb();
    mutex_unlock(&proc->mm_lock);
    panic("fork_memory: todo: un-cow pages on alloc failure");
    return false;
}

void pagefault(struct exception *e)
{
    unsigned int offset, size;
    uint32_t page;
    int ret;

    if (!proc) {
        dump_exception(e);
        panic("page fault during memory initialization");
    }
    ENABLE_INTERRUPTS;

    printk("page fault: pid %d: %s 0x%x\n", proc->pid,
           e->err & PF_WRITE ? "write" : "read", e->cr2);
    
    page = PAGE_BASE(e->cr2);
    mutex_lock(&proc->mm_lock);

    if ((e->err & PF_PRESENT) == 0) {
        if (page >= proc->text_base && page < proc->data_base
            && (e->err & PF_WRITE) == 0)
        {
            /* Lazily load text segment of executable. */
            if (!alloc_page(page, PAGE_USER | PAGE_WRITABLE))
                panic("page fault alloc failed");

            offset = page - proc->text_base;
            size = MIN(PAGE_SIZE, proc->hdr.text_size - offset);
            ret = iread(proc->exe, (void *)page,
                        offset + sizeof(struct exe_header), size);
            if (ret < size)
                panic("page fault read failed");
            
            set_page_writable(page, false);
            goto done;
        } else if (page >= proc->data_base && page < proc->data_top) {
            /* Lazily load data segment of executable. */
            if (!alloc_page(page, PAGE_USER | PAGE_WRITABLE))
                panic("page fault alloc failed");

            offset = page - proc->data_base;
            size = MIN(PAGE_SIZE, proc->hdr.data_size - offset);
            ret = iread(proc->exe, (void *)page,
                        offset + sizeof(struct exe_header)
                               + proc->hdr.text_size,
                        size);
            if (ret < size)
                panic("page fault read failed");

            goto done;
        } else if (page < 0xfffff000 && page >= proc->stack_base - PAGE_SIZE) {
            /* Lazily allocate a new page of the stack. */
            if (!alloc_page(page, PAGE_USER | PAGE_WRITABLE))
                panic("page fault alloc failed");

            proc->stack_base -= PAGE_SIZE;
            goto done;
        }
    } else if ((e->err & PF_WRITE) && (check_page(page) & PAGE_COPYONWRITE)) {
        /* Write to copy-on-write page. */
        mutex_lock(&pc_lock);
        if (pagecount[(vtophys(page)-HIMEM_BASE) >> 12] == 0) {
            mutex_unlock(&pc_lock);
            ptabs[TABENT(page)] &= ~PAGE_COPYONWRITE;
            ptabs[TABENT(page)] |= PAGE_WRITABLE;
        } else {
            pagecount[(vtophys(page)-HIMEM_BASE) >> 12]--;
            mutex_unlock(&pc_lock);
            if (!alloc_page(0xfffff000, PAGE_WRITABLE))
                panic("out of memory");
            flush_tlb();
            memcpy((void *)0xfffff000, (void *)page, PAGE_SIZE);
            ptabs[TABENT(page)] = vtophys(0xfffff000)
                                  | PAGE_PRESENT | PAGE_USER | PAGE_WRITABLE;
        }
        goto done;
    }

    if (e->err & PF_USER) {
        dump_exception(e);
        thread->signal |= (1 << SIGSEGV);
    } else {
        dump_exception(e);
        panic("unexpected kmode page fault");
    }

    return;

done:
    flush_tlb();
    mutex_unlock(&proc->mm_lock);
}
