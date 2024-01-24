ENTRY (_start)

SECTIONS {
	. = 0xC0100000;

	_kernel_start = .;

	.text ALIGN(4K) : AT (ADDR(.text) - 0xC0000000) {
		*(.multiboot)
		*(.text)
	}

	.rodata ALIGN(4K) : AT (ADDR(.rodata) - 0xC0000000) {
		_start_ctors = .;
		*(.init_array)
		_end_ctors = .;

		_start_dtors = .;
		*(.fini_array)
		_end_dtors = .;

		*(.rodata)
	}

	.data ALIGN(4K) : AT (ADDR(.data) - 0xC0000000) {
		*(.data)
	}

	.bss ALIGN(4K) : AT (ADDR(.bss) - 0xC0000000) {
		*(COMMON)
		*(.bss)
		*(.stack)
	}

	_kernel_end = .;
}