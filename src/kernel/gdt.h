#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// GDT Entry structure
struct gdt_entry {
    uint16_t limit_low;    // Lower 16 bits of limit
    uint16_t base_low;     // Lower 16 bits of base
    uint8_t base_middle;   // Next 8 bits of base
    uint8_t access;        // Access flags
    uint8_t granularity;   // Granularity and limit_high
    uint8_t base_high;     // Last 8 bits of base
} __attribute__((packed));

// GDT Pointer structure
struct gdt_ptr {
    uint16_t limit;        // Upper 16 bits of all selector limits
    uint32_t base;         // Address of the first gdt_entry
} __attribute__((packed));

// GDT segment selectors
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE   0x18
#define GDT_USER_DATA   0x20
#define GDT_TSS         0x28

// Access byte flags
#define GDT_ACCESS_PRESENT    0x80
#define GDT_ACCESS_RING0      0x00
#define GDT_ACCESS_RING3      0x60
#define GDT_ACCESS_CODE       0x18
#define GDT_ACCESS_DATA       0x10
#define GDT_ACCESS_READWRITE  0x02
#define GDT_ACCESS_EXECUTE    0x08

// Granularity flags
#define GDT_GRAN_4K          0x80
#define GDT_GRAN_32BIT       0x40

// Function declarations
void init_gdt(void);
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void verify_gdt(void);

#endif // GDT_H 