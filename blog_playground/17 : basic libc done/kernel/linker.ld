ENTRY(start)

SECTIONS
{
    . = 0x9000;

    _kernel_start = .;

    .text :
    {
        *(.text)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
    }

    _kernel_end = .;
}
