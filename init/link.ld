OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)
ENTRY(start)

SECTIONS
{
    . = 0x7ffffff0;

    .text :
    {
        LONG(0xcafebabe);
        LONG(0);
        LONG(_text_end - _text);
        LONG(_data_end - _data);

        _text = .;
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
    }

    _text_end = .;

    .data ALIGN(4K) :
    {
        _data = .;
        *(.data)
    }

    .bss :
    {
        *(.bss)
    }

    _data_end = .;

    /DISCARD/ :
    {
        *(.fini_array*)
        *(.comment)
        *(.eh_frame)
    }
}
