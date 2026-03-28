OUTPUT_FORMAT("binary")
ENTRY(entry)

SECTIONS
{
    . = 0x0000;

    .text : ALIGN(16)
    {
        *(.text*)
    }

    .rodata : ALIGN(16)
    {
        *(.rodata*)
    }

    .data : ALIGN(16)
    {
        *(.data*)
    }

    .bss : ALIGN(16)
    {
        *(.bss*)
        *(COMMON)
    }
}
