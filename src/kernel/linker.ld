OUTPUT_FORMAT("elf32-i386")
ENTRY(start)

SECTIONS
{
	. = 0x00000000;

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
