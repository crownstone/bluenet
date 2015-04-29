/**********************************************************************************************************************
 * Sections for the linker
 * The total amount of RAM available with the nRF51822 is 16kB (16384 bytes), so end of SP is at 0x20004000.
 * The SoftDevice takes a hugh part of the RAM at the beginning (which ends at ORIGIN of RAM for the application).
 * Use "make analyze" to see how much the .bss usurps: 2656 bytes and the data(VMA) part: 112 bytes.
 * The S130 on the moment uses 0x2800 bytes, 10240 bytes of the RAM, 10kB! So that leaves us less than 6kB...!
 * This leaves us a total of 13616-10240 bytes, so we can only build a small heap.
 * On the moment we need a heap of around 2kB. It will start
 * around 10240+2656+112 = 0x200032d0.
 * Then we have only around 1kB left for the stack!!!
 *********************************************************************************************************************/
HEAP_SIZE = 2700;

SECTIONS
{

	.text : {
		. = 0;
		__Vectors = .;
		KEEP(*(.vectors))
		*(.startup*)
		. = 0x400;
		*(.text*)
		*(.gnu.linkonce.t*)
		*(.rodata* .gnu.linkonce.r.*)
		. = ALIGN(4);
		KEEP(*(.init))
		*(.ARM.extab* .gnu.linkonce.armextab.*)
 		*(.gcc_except_table)
		. = ALIGN(4);
		__preinit_array_start = .;
		KEEP (*(.preinit_array))
		__preinit_array_end = .;
		__init_array_start = .;
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		__init_array_end = .;

	} > FLASH = 0xFF

	.ARM.exidx : {
		__exidx_start = .;
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
		__exidx_end = .;
	} > FLASH
	
	.eh_frame : {
		*(.eh_frame);

	} > FLASH
	

	_etext = .;
	__text_end__ = .;

	.data : AT (_etext) {
		. = ALIGN(4);
		_sdata = .;
		__data_start__ = .;
		*(.data* .gnu.linkonce.d.*)
		. = ALIGN(4);
		_edata = .;
		__data_end__ = .;
	} > RAM /*AT>FLASH */
	PROVIDE(_sdata = _sdata);
	PROVIDE(_edata = _edata);

	.noinit (NOLOAD) : {
		*(.noinit*)
	} > RAM

	.bss : {
		. = ALIGN(4);
		_sbss = .;
		PROVIDE (__bss_start__ = _sbss);
		*(.shbss)
		*(.bss* .gnu.linkonce.b.*)
		*(COMMON)
		*(.ram.b .bss.ram)
		. = ALIGN(4);
		_ebss = .;
		__bss_end__ = .;
	} > RAM
	end = .;
	_end = .;
	__end = .;
	PROVIDE (__end__ = end);

	.heap :
	{
		_heap_start = .;
		. = . + HEAP_SIZE;
		_heap_end = .;
	} > RAM

	_estack = ORIGIN(RAM) + LENGTH(RAM);

}

