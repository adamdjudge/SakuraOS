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

#define ARG_MAX 1024

static char args_buf[ARG_MAX];
static char env_buf[ARG_MAX];
static spinlock_t buf_lock;

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

    if (*((uint32_t *)ehdr->e_ident) != ELFMAG_WORD)
        printk("verify_elf: not an ELF file\n");
    else if (ehdr->e_ident[EI_CLASS] != ELFCLASS32)
        printk("verify_elf: e_ident[EI_CLASS] invalid for i386\n");
    else if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
        printk("verify_elf: e_ident[EI_DATA] invalid for i386\n");
    else if (ehdr->e_type != ET_EXEC)
        printk("verify_elf: not an executable\n");
    else if (ehdr->e_machine != EM_386)
        printk("verify_elf: e_machine is not i386\n");
    else if (ehdr->e_phnum == 0)
        printk("verify_elf: file contains no program headers\n");
    else
        return 0;

    return -ENOEXEC;
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

static bool copy_to_buf(char **arr, char *buf, int *len, int *count)
{
    char *s;

    *len = 0;
    *count = 0;
    if (!arr)
        return true;

    while (*arr) {
        *count = *count + 1;
        s = *arr++;
        while (*s) {
            if ((*len)++ == ARG_MAX)
                return false;
            *buf++ = *s++;
        }
        if ((*len)++ == ARG_MAX)
            return false;
        *buf++ = '\0';
    }

    return true;
}

static uint32_t push_array(uint32_t sp, char *buf, int len, int count)
{
    char *s, **arr;

    sp -= len;
    s = (char *)sp;
    memcpy(s, buf, len);

    sp -= (count+1) * sizeof(char *);
    arr = (char **)sp;
    while (count-- > 0) {
        *arr++ = s;
        while (*s)
            s++;
        s++;
    }

    return sp;
}

// TODO: make a centralized perror() instead
static char *errmsg(int errno)
{
    switch (errno) {
    case -ENOEXEC: return "bad file";
    case -EIO: return "I/O error";
    case -EACCESS: return "permission denied";
    case -E2BIG: return "argument/environment too big";
    }
    return "other";
}

int sys_execve(struct exception *e)
{
    int ret, i, args_count, args_len, env_count, env_len;
    struct inode *exe = NULL;
    struct elf32_ehdr ehdr;
    char *pathname = (char *)e->ebx;
    char **argv = (char **)e->ecx;
    char **envp = (char **)e->edx;
    uint32_t new_argv, new_envp, *sp;

    ret = ilookup(&exe, pathname);
    if (ret) {
        printk("execve (%d): could not open '%s'\n", proc->pid, pathname);
        return ret;
    }

    /* TODO: permission check */

    if ((ret = verify_elf(exe, &ehdr)) < 0)
        goto error;

    spin_lock(&buf_lock);
    if (!copy_to_buf(argv, args_buf, &args_len, &args_count)
        || !copy_to_buf(envp, env_buf, &env_len, &env_count))
    {
        ret = -E2BIG;
        goto error;
    }

    printk("execve (%d): %s\n", proc->pid, pathname);
    sched_stop_other_threads();
    mm_free_proc_memory();
    if (proc->exe)
        iput(proc->exe);

    if ((ret = load_elf(exe, &ehdr)) < 0)
        goto error;
    mm_add_mapping(0xffffe000, PAGE_SIZE, VMAP_WRITABLE | VMAP_STACK,
                   0, 0, NULL);
    new_argv = push_array(0xfffff000, args_buf, args_len, args_count);
    new_envp = push_array(new_argv, env_buf, env_len, env_count);
    spin_unlock(&buf_lock);

    proc->exe = exe;
    proc->rtime = 0;
    proc->ktime = 0;
    proc->utime = 0;
    thread->tid = 1;
    proc->next_tid = 2;

    for (i = 0; i < 32; i++) {
        if (proc->sigdisp[i] > SIG_IGN)
            proc->sigdisp[i] = SIG_DFL;
    }
    for (i = 0; i < OPEN_MAX; i++) {
        if (proc->files[i] && (proc->files[i]->flags & O_CLOEXEC))
            close(i);
    }

    sp = (uint32_t *)new_envp;
    *(--sp) = new_envp;
    *(--sp) = new_argv;
    e->esp = (uint32_t)sp;

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
    e->eflags = (1 << 9);
    e->eip = ehdr.e_entry;
    return 0;

error:
    printk("execve (%d): %s\n", proc->pid, errmsg(ret));
    if (exe)
        iput(exe);
    spin_unlock(&buf_lock);
    return ret;
}
