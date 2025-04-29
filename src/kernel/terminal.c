#include <stdint.h>
#include <stddef.h>
#include "terminal.h"
#include "io.h"
#include "uart.h"

// Memory barrier for VGA access
static inline void vga_memory_barrier(void) {
    asm volatile("" ::: "memory");
}

// Screen structure to hold state
typedef struct {
    uint16_t buffer[VGA_HEIGHT * VGA_WIDTH];
    size_t row;
    size_t column;
    uint8_t color;
} screen_t;

// Global screen state
static screen_t screen;
static volatile uint16_t* vga_buffer = (volatile uint16_t*)VGA_ADDRESS;

// Helper function to safely write to VGA buffer
static void safe_vga_write(size_t index, uint16_t value) {
    if (index < VGA_HEIGHT * VGA_WIDTH) {
        // Ensure compiler doesn't reorder memory operations
        asm volatile("" ::: "memory");
        // Write to VGA buffer
        vga_buffer[index] = value;
        // Ensure write is complete
        asm volatile("" ::: "memory");
    }
}

void terminal_setcolor(uint8_t color) {
    screen.color = color;
}

void terminal_scroll(void) {
    // Move all lines up by one
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t current = y * VGA_WIDTH + x;
            const size_t next = (y + 1) * VGA_WIDTH + x;
            screen.buffer[current] = screen.buffer[next];
            safe_vga_write(current, screen.buffer[current]);
        }
    }
    
    // Clear the last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        screen.buffer[index] = vga_entry(' ', screen.color);
        safe_vga_write(index, screen.buffer[index]);
    }
    
    screen.row = VGA_HEIGHT - 1;
}

void terminal_clear(void) {
    uart_write_string("Starting terminal_clear...\n");
    
    // Clear the screen
    uart_write_string("Clearing screen buffer...\n");
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        uart_write_string("Clearing row ");
        uart_write_hex(y);
        uart_write_string("\n");
        
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            screen.buffer[index] = vga_entry(' ', screen.color);
            safe_vga_write(index, screen.buffer[index]);
        }
    }
    
    uart_write_string("Screen buffer cleared\n");
    
    screen.row = 0;
    screen.column = 0;
    
    uart_write_string("Terminal_clear complete\n");
}

void terminal_initialize(void) {
    uart_write_string("Starting terminal initialization...\n");
    
    // Initialize screen structure
    uart_write_string("Initializing screen structure...\n");
    screen.row = 0;
    screen.column = 0;
    screen.color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    // Initialize VGA buffer pointer
    uart_write_string("Setting up VGA buffer pointer...\n");
    vga_buffer = (volatile uint16_t*)VGA_ADDRESS;
    uart_write_string("VGA buffer pointer set to: ");
    uart_write_hex((uint32_t)vga_buffer);
    uart_write_string("\n");
    
    // Initialize local buffer in smaller chunks
    uart_write_string("Initializing local buffer...\n");
    const size_t chunk_size = 4;  // Initialize 4 characters at a time for more granular control
    
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        uart_write_string("Starting row ");
        uart_write_hex(y);
        uart_write_string("\n");
        
        for (size_t x = 0; x < VGA_WIDTH; x += chunk_size) {
            uart_write_string("  Initializing chunk at x=");
            uart_write_hex(x);
            uart_write_string("\n");
            
            size_t remaining = VGA_WIDTH - x;
            size_t current_chunk = (remaining < chunk_size) ? remaining : chunk_size;
            
            for (size_t i = 0; i < current_chunk; i++) {
                const size_t index = y * VGA_WIDTH + (x + i);
                if (index < VGA_HEIGHT * VGA_WIDTH) {
                    screen.buffer[index] = vga_entry(' ', screen.color);
                    safe_vga_write(index, screen.buffer[index]);
                }
            }
        }
    }
    
    uart_write_string("Local buffer initialized\n");
    uart_write_string("Terminal initialization complete\n");
}

void terminal_putchar(char c) {
    if (c == '\n') {
        screen.column = 0;
        if (++screen.row == VGA_HEIGHT) {
            terminal_scroll();
        }
        return;
    }
    
    if (c == '\t') {
        screen.column = (screen.column + 8) & ~(8 - 1);
        if (screen.column >= VGA_WIDTH) {
            screen.column = 0;
            if (++screen.row == VGA_HEIGHT) {
                terminal_scroll();
            }
        }
        return;
    }
    
    if (c == '\b') {
        if (screen.column > 0) {
            screen.column--;
        } else if (screen.row > 0) {
            screen.row--;
            screen.column = VGA_WIDTH - 1;
        }
        const size_t index = screen.row * VGA_WIDTH + screen.column;
        screen.buffer[index] = vga_entry(' ', screen.color);
        safe_vga_write(index, screen.buffer[index]);
        return;
    }

    const size_t index = screen.row * VGA_WIDTH + screen.column;
    screen.buffer[index] = vga_entry(c, screen.color);
    safe_vga_write(index, screen.buffer[index]);

    if (++screen.column == VGA_WIDTH) {
        screen.column = 0;
        if (++screen.row == VGA_HEIGHT) {
            terminal_scroll();
        }
    }
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

// Custom strlen implementation
static size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

void terminal_setcursor(size_t x, size_t y) {
    if (x >= VGA_WIDTH) x = VGA_WIDTH - 1;
    if (y >= VGA_HEIGHT) y = VGA_HEIGHT - 1;
    screen.column = x;
    screen.row = y;
} 