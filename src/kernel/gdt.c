#include "gdt.h"
#include "io.h"
#include <stddef.h>
#include "uart.h"
#include "terminal.h"

// GDT entries
struct gdt_entry gdt[6];
struct gdt_ptr gp;

// Assembly function to load the GDT
extern void gdt_flush(uint32_t);

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    uart_write_string("Setting GDT gate ");
    uart_write_hex(num);
    uart_write_string("\n");

    // Setup the descriptor base address
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    // Setup the descriptor limits
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    // Finally, set up the granularity and access flags
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

void verify_gdt(void) {
    struct gdt_entry* gdt_at_800 = (struct gdt_entry*)0x00000800;
    
    terminal_writestring("\nGDT Verification at 0x00000800:\n");
    terminal_writestring("Entry  Base      Limit     Access  Gran\n");
    terminal_writestring("----------------------------------------\n");
    
    for (int i = 0; i < 6; i++) {
        uint32_t base = (uint32_t)gdt_at_800[i].base_high << 24 | 
                       (uint32_t)gdt_at_800[i].base_middle << 16 | 
                       (uint32_t)gdt_at_800[i].base_low;
        uint32_t limit = (uint32_t)((gdt_at_800[i].granularity & 0x0F) << 16) | 
                        (uint32_t)gdt_at_800[i].limit_low;
        
        terminal_writehex(i);
        terminal_writestring("    ");
        terminal_writehex(base);
        terminal_writestring("  ");
        terminal_writehex(limit);
        terminal_writestring("  ");
        terminal_writehex(gdt_at_800[i].access);
        terminal_writestring("  ");
        terminal_writehex(gdt_at_800[i].granularity);
        terminal_writestring("\n");
    }
}

void init_gdt(void) {
    uart_write_string("Starting GDT initialization\n");

    // Setup the GDT pointer and limit
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base = 0x00000800;  // Set GDT at required address
    uart_write_string("GDT pointer set to 0x00000800\n");

    // Our NULL descriptor
    uart_write_string("Setting NULL descriptor\n");
    gdt_set_gate(0, 0, 0, 0, 0);

    // Kernel Code Segment
    uart_write_string("Setting Kernel Code Segment\n");
    gdt_set_gate(1, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE | GDT_ACCESS_EXECUTE,
        GDT_GRAN_4K | GDT_GRAN_32BIT);

    // Kernel Data Segment
    uart_write_string("Setting Kernel Data Segment\n");
    gdt_set_gate(2, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA | GDT_ACCESS_READWRITE,
        GDT_GRAN_4K | GDT_GRAN_32BIT);

    // User Code Segment
    uart_write_string("Setting User Code Segment\n");
    gdt_set_gate(3, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_CODE | GDT_ACCESS_EXECUTE,
        GDT_GRAN_4K | GDT_GRAN_32BIT);

    // User Data Segment
    uart_write_string("Setting User Data Segment\n");
    gdt_set_gate(4, 0, 0xFFFFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DATA | GDT_ACCESS_READWRITE,
        GDT_GRAN_4K | GDT_GRAN_32BIT);

    // TSS Segment (placeholder for now)
    uart_write_string("Setting TSS Segment\n");
    gdt_set_gate(5, 0, 0, 0, 0);

    // Copy GDT to the required address
    uart_write_string("Copying GDT to 0x00000800\n");
    volatile uint8_t *gdt_ptr = (volatile uint8_t *)gp.base;
    const uint8_t *src_ptr = (const uint8_t *)gdt;
    
    // Use volatile to ensure the compiler doesn't optimize out the memory writes
    for (size_t i = 0; i < sizeof(gdt); i++) {
        gdt_ptr[i] = src_ptr[i];
    }
    uart_write_string("GDT copy completed\n");

    // Flush the old GDT and load the new one
    uart_write_string("Flushing GDT\n");
    gdt_flush((uint32_t)&gp);
    uart_write_string("GDT initialization complete\n");

    // Verify the GDT contents
    verify_gdt();
} 