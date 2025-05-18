/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

#include <kernel.h>
#include <elf.h>
#include <exception.h>
#include <mm.h>
#include <sched.h>
#include <fs.h>
#include <signal.h>

static int verify_elf(struct inode *exe, struct elf32_ehdr *ehdr)
{
    int ret;

    if (exe->size < sizeof(*ehdr))
        return -ENOEXEC;

    ret = iread(exe, (char *)ehdr, 0, sizeof(*ehdr));
    if (ret < 0)
        return ret;
    else if (ret < sizeof(*ehdr))
        return -EIO;

    if (*((uint32_t *)ehdr->e_ident) != ELFMAG_WORD) {
        printk("verify_elf: not an ELF file\n");
        return -ENOEXEC;
    } else if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
        printk("verify_elf: e_ident[EI_CLASS] invalid for i386\n");
        return -ENOEXEC;
    } else if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        printk("verify_elf: e_ident[EI_DATA] invalid for i386\n");
        return -ENOEXEC;
    } else if (ehdr->e_type != ET_EXEC) {
        printk("verify_elf: not an executable\n");
        return -ENOEXEC;
    } else if (ehdr->e_machine != EM_386) {
        printk("verify_elf: e_machine is not i386\n");
        return -ENOEXEC;
    } else if (ehdr->e_phnum == 0) {
        printk("verify_elf: file contains no program headers\n");
        return -ENOEXEC;
    }

    return 0;
}

static int load_elf(struct inode *exe, struct elf32_ehdr *ehdr)
{
    int i, ret;
    unsigned int offset;
    struct elf32_phdr phdr;

    offset = ehdr->e_phoff;
    for (i = 0; i < ehdr->e_phnum; i++) {
        if (exe->size < offset + sizeof(phdr))
            return -ENOEXEC;

        ret = iread(exe, (char *)&phdr, offset, sizeof(phdr));
        if (ret < 0)
            return ret;
        else if (ret < sizeof(phdr))
            return -EIO;

        if (phdr.p_type != PT_LOAD)
            continue;
        else if (phdr.p_align != PAGE_SIZE || (phdr.p_vaddr & PAGE_MASK) != 0) {
            printk("load_elf: misaligned program segment\n");
            return -ENOEXEC;
        }
        
        mm_add_mapping(phdr.p_vaddr, phdr.p_memsz,
                       (phdr.p_flags & PF_W) ? VMAP_WRITABLE : VMAP_READONLY,
                       phdr.p_offset, phdr.p_filesz, exe);
        
        offset += ehdr->e_phentsize;
    }

    return 0;
}

int sys_execve(struct exception *e)
{
    int ret, i;
    struct inode *exe;
    struct elf32_ehdr ehdr;
    char *filename = (char *)e->ebx;

    ret = ilookup(&exe, filename);
    if (ret) {
        printk("execve: error: could not open '%s'\n", filename);
        return ret;
    }

    /* TODO: permission check */

    ret = verify_elf(exe, &ehdr);
    if (ret == -ENOEXEC)
        goto bad_file;
    else if (ret < 0)
        goto read_error;

    printk("execve: pid %d: %s\n", proc->pid, filename);

    sched_stop_other_threads();
    mm_free_proc_memory();
    if (proc->exe)
        iput(proc->exe);

    ret = load_elf(exe, &ehdr);
    if (ret == -ENOEXEC)
        goto bad_file;
    else if (ret < 0)
        goto read_error;
    mm_add_mapping(0xffffe000, PAGE_SIZE, VMAP_WRITABLE | VMAP_STACK,
                   0, 0, NULL);

    proc->exe = exe;
    proc->rtime = 0;
    proc->ktime = 0;
    proc->utime = 0;
    for (i = 0; i < 32; i++) {
        if (proc->sigdisp[i] > SIG_IGN)
            proc->sigdisp[i] = SIG_DFL;
    }
    thread->tid = 1;
    proc->next_tid = 2;

    e->cs = 0x1B;
    e->ds = 0x23;
    e->es = 0x23;
    e->fs = 0x23;
    e->gs = 0x23;
    e->ss = 0x23;
    e->eax = 0;
    e->ebx = 0;
    e->ecx = 0;
    e->edx = 0;
    e->edi = 0;
    e->esi = 0;
    e->ebp = 0;
    e->esp = 0xfffff000;
    e->eflags = (1 << 9);
    e->eip = ehdr.e_entry;
    return 0;

bad_file:
    printk("execve: error: bad file '%s'\n", filename);
    return ret;
read_error:
    printk("execve: error: failed to read '%s'\n", filename);
    return ret;
}
