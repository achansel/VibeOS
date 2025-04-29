#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
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
    bool prompt_initialized;
} screen_t;

// Global screen state
static screen_t screens[NUM_SCREENS];
static uint8_t current_screen = 0;
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

// Helper function to get current screen
static screen_t* get_current_screen(void) {
    return &screens[current_screen];
}

void terminal_setcolor(uint8_t color) {
    get_current_screen()->color = color;
}

void terminal_scroll(void) {
    screen_t* screen = get_current_screen();
    
    // Move all lines up by one
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t current = y * VGA_WIDTH + x;
            const size_t next = (y + 1) * VGA_WIDTH + x;
            screen->buffer[current] = screen->buffer[next];
            safe_vga_write(current, screen->buffer[current]);
        }
    }
    
    // Clear the last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        screen->buffer[index] = vga_entry(' ', screen->color);
        safe_vga_write(index, screen->buffer[index]);
    }
    
    screen->row = VGA_HEIGHT - 1;
}

void terminal_clear(void) {
    screen_t* screen = get_current_screen();
    
    // Clear the screen
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            screen->buffer[index] = vga_entry(' ', screen->color);
            safe_vga_write(index, screen->buffer[index]);
        }
    }
    
    screen->row = 0;
    screen->column = 0;
}

void terminal_initialize(void) {
    // Initialize all screens
    for (uint8_t i = 0; i < NUM_SCREENS; i++) {
        screens[i].row = 0;
        screens[i].column = 0;
        screens[i].color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        screens[i].prompt_initialized = false;
        
        // Initialize VGA buffer for this screen
        for (size_t y = 0; y < VGA_HEIGHT; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                const size_t index = y * VGA_WIDTH + x;
                screens[i].buffer[index] = vga_entry(' ', screens[i].color);
                safe_vga_write(index, screens[i].buffer[index]);
            }
        }
    }
    
    // Set current screen to 0 and initialize its prompt
    current_screen = 0;
    screens[0].prompt_initialized = true;
    
    // Enable and position cursor
    terminal_enable_cursor();
    terminal_update_cursor();
}

void terminal_disable_cursor(void) {
    // VGA cursor control ports
    const uint16_t VGA_CTRL_REGISTER = 0x3D4;
    const uint16_t VGA_DATA_REGISTER = 0x3D5;
    
    // Disable cursor by setting the maximum scan line to 0
    outb(VGA_CTRL_REGISTER, 0x0A);
    outb(VGA_DATA_REGISTER, 0x20);
    
    // Small delay to ensure register write completes
    for (volatile int i = 0; i < 100; i++);
}

void terminal_enable_cursor(void) {
    // VGA cursor control ports
    const uint16_t VGA_CTRL_REGISTER = 0x3D4;
    const uint16_t VGA_DATA_REGISTER = 0x3D5;
    
    // Set cursor start line to 0 (top of character)
    outb(VGA_CTRL_REGISTER, 0x0A);
    outb(VGA_DATA_REGISTER, 0x00);
    
    // Small delay to ensure register write completes
    for (volatile int i = 0; i < 100; i++);
    
    // Set cursor end line to 15 (bottom of character)
    outb(VGA_CTRL_REGISTER, 0x0B);
    outb(VGA_DATA_REGISTER, 0x0F);
    
    // Small delay to ensure register write completes
    for (volatile int i = 0; i < 100; i++);
}

void terminal_update_cursor(void) {
    // VGA cursor control ports
    const uint16_t VGA_CTRL_REGISTER = 0x3D4;
    const uint16_t VGA_DATA_REGISTER = 0x3D5;
    
    screen_t* screen = get_current_screen();
    uint16_t pos = screen->row * VGA_WIDTH + screen->column;
    
    // Update cursor position (low byte)
    outb(VGA_CTRL_REGISTER, 0x0F);
    outb(VGA_DATA_REGISTER, (uint8_t)(pos & 0xFF));
    
    // Small delay to ensure register write completes
    for (volatile int i = 0; i < 100; i++);
    
    // Update cursor position (high byte)
    outb(VGA_CTRL_REGISTER, 0x0E);
    outb(VGA_DATA_REGISTER, (uint8_t)((pos >> 8) & 0xFF));
    
    // Small delay to ensure register write completes
    for (volatile int i = 0; i < 100; i++);
}

void terminal_putchar(char c) {
    screen_t* screen = get_current_screen();
    
    if (c == '\n') {
        screen->column = 0;
        if (++screen->row == VGA_HEIGHT) {
            terminal_scroll();
        }
        terminal_update_cursor();
        return;
    }
    
    if (c == '\t') {
        screen->column = (screen->column + 8) & ~(8 - 1);
        if (screen->column >= VGA_WIDTH) {
            screen->column = 0;
            if (++screen->row == VGA_HEIGHT) {
                terminal_scroll();
            }
        }
        terminal_update_cursor();
        return;
    }
    
    if (c == '\b') {
        if (screen->column > 0) {
            screen->column--;
        } else if (screen->row > 0) {
            screen->row--;
            screen->column = VGA_WIDTH - 1;
        }
        const size_t index = screen->row * VGA_WIDTH + screen->column;
        screen->buffer[index] = vga_entry(' ', screen->color);
        safe_vga_write(index, screen->buffer[index]);
        terminal_update_cursor();
        return;
    }

    const size_t index = screen->row * VGA_WIDTH + screen->column;
    screen->buffer[index] = vga_entry(c, screen->color);
    safe_vga_write(index, screen->buffer[index]);

    if (++screen->column == VGA_WIDTH) {
        screen->column = 0;
        if (++screen->row == VGA_HEIGHT) {
            terminal_scroll();
        }
    }
    terminal_update_cursor();
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

void terminal_switch_screen(uint8_t screen_num) {
    if (screen_num >= NUM_SCREENS) {
        return;
    }
    
    // Save current screen state
    screen_t* current = get_current_screen();
    
    // Switch to new screen
    current_screen = screen_num;
    
    // Copy the new screen's buffer to VGA memory
    screen_t* new_screen = get_current_screen();
    
    for (size_t i = 0; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        safe_vga_write(i, new_screen->buffer[i]);
    }
    
    // Initialize prompt if this is the first time switching to this screen
    if (!new_screen->prompt_initialized) {
        new_screen->prompt_initialized = true;
        terminal_writestring("> ");
    }
    
    // Update cursor position for the new screen
    terminal_update_cursor();
} 