ENTRY(_start)

/* Memory layout */
MEMORY {
    /* First megabyte (reserved for BIOS, VGA, etc.) */
    lowmem (rwx) : ORIGIN = 0x0, LENGTH = 1M
    /* Kernel memory */
    kernel (rwx) : ORIGIN = 1M, LENGTH = 15M
}

SECTIONS {
    /* Kernel sections start at 1MB */
    . = 1M;

    .boot : {
        *(.multiboot)
    } > kernel :text

    .text : {
        *(.text)
    } > kernel :text

    .rodata : {
        *(.rodata)
    } > kernel :data

    .data : {
        *(.data)
    } > kernel :data

    .bss : {
        *(COMMON)
        *(.bss)
    } > kernel :bss

    /* End of kernel */
    . = ALIGN(4K);
    _kernel_end = .;
}

/* Memory permissions */
PHDRS {
    text PT_LOAD FLAGS(5);  /* R-X */
    data PT_LOAD FLAGS(6);  /* RW- */
    bss PT_LOAD FLAGS(6);   /* RW- */
} 