OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)
ENTRY(start)

SECTIONS
{
    . = 0x40000000;

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
        . = ALIGN(4K);
        *(.data)
    }

    .bss :
    {
        *(.bss)
    }

    /DISCARD/ :
    {
        *(.fini_array*)
        *(.comment)
        *(.eh_frame)
    }
}
