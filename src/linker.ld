ENTRY(_start)

/* Memory layout */
MEMORY {
    /* First megabyte (reserved for BIOS, VGA, etc.) */
    lowmem (rwx) : ORIGIN = 0x0, LENGTH = 1M
    /* Kernel memory */
    kernel (rwx) : ORIGIN = 1M, LENGTH = 15M
}

SECTIONS {
    /* VGA buffer must be at 0xB8000 */
    .vga_buffer 0xB8000 : {
        *(.vga_buffer)
    } > lowmem

    /* Kernel sections start at 1MB */
    . = 1M;

    .boot : {
        *(.multiboot)
    } > kernel

    .text : {
        *(.text)
    } > kernel

    .rodata : {
        *(.rodata)
    } > kernel

    .data : {
        *(.data)
    } > kernel

    .bss : {
        *(COMMON)
        *(.bss)
    } > kernel

    /* End of kernel */
    . = ALIGN(4K);
    _kernel_end = .;
} 