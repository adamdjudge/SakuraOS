/**
 * The SakuraOS Bootloader
 * Copyright 2026 Adam Judge
 */

#include "bootloader.h"
#include "string.h"
#include "elf.h"
#include "fs.h"

static uint32_t load_top = 0;

/* Loads the specified ELF executable in the "/boot" directory as the kernel,
 * and returns the start address. */
uint32_t load_kernel(char *name)
{
    struct inode file;
    if (!get_bootdir_inode(&file, name)) {
        printf("File '/boot/%s' not found.\n", name);
        return 0;
    }
    if (file.size < sizeof(struct elf32_ehdr))
        goto invalid;

    struct elf32_ehdr ehdr;
    if (!fileread(&file, &ehdr, 0, sizeof(ehdr)))
        goto read_error;
    if (*((uint32_t *)&ehdr.e_ident) != ELFMAG_WORD)
        goto invalid;
    else if (ehdr.e_ident[EI_CLASS] != ELFCLASS32)
        goto invalid;
    else if (ehdr.e_ident[EI_DATA] != ELFDATA2LSB)
        goto invalid;
    else if (ehdr.e_type != ET_EXEC)
        goto invalid;
    else if (ehdr.e_machine != EM_386)
        goto invalid;
    else if (ehdr.e_phnum == 0)
        goto invalid;

    unsigned int phoff = ehdr.e_phoff;
    for (int i = 0; i < ehdr.e_phnum; i++) {
        if (file.size < phoff + sizeof(struct elf32_phdr))
            goto invalid;

        struct elf32_phdr phdr;
        if (!fileread(&file, &phdr, phoff, sizeof(phdr)))
            goto read_error;
        phoff += ehdr.e_phentsize;
        
        if (phdr.p_type != PT_LOAD)
            continue;
        else if (phdr.p_paddr % 4096 != 0)
            goto invalid;
        else if (file.size < phdr.p_offset + phdr.p_filesz)
            goto invalid;
        if (!fileread(&file, (void *)phdr.p_paddr, phdr.p_offset,
                      phdr.p_filesz))
            goto read_error;
        
        load_top = MAX(load_top, phdr.p_paddr + phdr.p_memsz);
    }

    return ehdr.e_entry;

invalid:
    printf("File '/boot/%s' is not a valid ELF executable.\n", name);
    return 0;
read_error:
    printf("Read error while loading '/boot/%s'.\n", name);
    return 0;
}
