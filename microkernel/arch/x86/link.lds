OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)
ENTRY(_entry)

SECTIONS
{
    . = 0x100000;
    __kernel_base = .;

    .text :
    {
        *(.multiboot)
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
    }

    .data ALIGN(4K) :
    {
        __kernel_rw = .;
        *(.data)
    }

    .bss :
    {
        __kernel_bss = .;
        *(.bss)
    }

    __kernel_end = .;

    /DISCARD/ :
    {
        *(.fini_array*)
        *(.comment)
        *(.eh_frame)
    }
}
