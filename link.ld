OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)
ENTRY(kernel_entry)

SECTIONS
{
    . = 0x8000;
    _kernel_base = .;

    .text :
    {
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
    }

    .data ALIGN(4K) :
    {
        _kernel_rw = .;
        *(.data)
    }

    .bss :
    {
        _kernel_bss = .;
        *(.bss)
    }

    _kernel_end = .;

    /DISCARD/ :
    {
        *(.fini_array*)
        *(.comment)
        *(.eh_frame)
    }
}
