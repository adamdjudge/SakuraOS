/**
 * The SakuraOS Kernel
 * Copyright 2025 Adam Judge
 */

/* ======== ELF HEADER DEFINITIONS ======== */

#define EI_NIDENT 16

struct elf32_ehdr {
    uint8_t e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

/**
 * e_type values
 */
#define ET_NONE  0
#define ET_REL   1
#define ET_EXEC  2
#define ET_DYN   3
#define ET_CORE  4

/**
 * e_machine values
 * (We don't care about anything other than i386 here)
 */
#define EM_386 3

/**
 * e_version values
 */
#define EV_NONE     0
#define EV_CURRENT  1

/**
 * e_ident indices
 */
enum {
    EI_MAG0,
    EI_MAG1,
    EI_MAG2,
    EI_MAG3,
    EI_CLASS,
    EI_DATA,
    EI_VERSION,
    EI_PAD,
};

/**
 * ELF magic number
 */
#define ELFMAG0      0x7f
#define ELFMAG1      'E'
#define ELFMAG2      'L'
#define ELFMAG3      'F'
#define ELFMAG_WORD  0x464c457f

/**
 * e_ident[EI_CLASS] values
 */
#define ELFCLASSNONE  0
#define ELFCLASS32    1
#define ELFCLASS64    2

/**
 * e_ident[EI_DATA] values
 */
#define ELFDATANONE  0
#define ELFDATA2LSB  1
#define ELFDATA2MSB  2

/* ======== PROGRAM HEADER DEFINITIONS ======== */

struct elf32_phdr {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};

/**
 * p_type values
 */
enum {
    PT_NULL,
    PT_LOAD,
    PT_DYNAMIC,
    PT_INTERP,
    PT_NOTE,
    PT_SHLIB,
    PT_PHDR,
};

/**
 * p_flags values
 */
#define PF_R  0x4
#define PF_W  0x2
#define PF_X  0x1
