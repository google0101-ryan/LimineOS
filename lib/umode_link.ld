ENTRY(_start)
SECTIONS
{

	. = 0xffffe00000000000;
	__start = .;



	.text ALIGN(4K) :
	{
		*(.text)    
        *(.text*)
		*(.init)
		*(.fini)
		*(.ctors)
		*(.dtors)
	}

	.rodata ALIGN(4K) : 
	{
        *(.ctors)
		*(.rodata)
	}

	.data ALIGN(4K) : 
	{
		*(.data)
	}

	.bss ALIGN(4K) : 
    {
		*(COMMON)
		*(.bss)
	}

	__end = .;
}