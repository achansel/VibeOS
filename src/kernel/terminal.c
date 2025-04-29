#include <stdint.h>
#include <stddef.h>
#include "terminal.h"
#include "io.h"

// Screen structure to hold state for each virtual screen
typedef struct {
    uint16_t buffer[VGA_HEIGHT * VGA_WIDTH];
    size_t row;
    size_t column;
    uint8_t color;
} screen_t;

// Global screen state
static screen_t screens[NUM_SCREENS];
static size_t current_screen = 0;
static uint16_t* vga_buffer __attribute__((section(".vga_buffer"))) = (uint16_t*)VGA_ADDRESS;

void terminal_setcolor(uint8_t color) {
    screens[current_screen].color = color;
}

void terminal_scroll(void) {
    screen_t* screen = &screens[current_screen];
    
    // Move all lines up by one
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t current = y * VGA_WIDTH + x;
            const size_t next = (y + 1) * VGA_WIDTH + x;
            screen->buffer[current] = screen->buffer[next];
        }
    }
    
    // Clear the last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        screen->buffer[index] = vga_entry(' ', screen->color);
    }
    
    screen->row = VGA_HEIGHT - 1;
}

void terminal_clear(void) {
    screen_t* screen = &screens[current_screen];
    
    // Clear the screen
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            screen->buffer[index] = vga_entry(' ', screen->color);
        }
    }
    
    screen->row = 0;
    screen->column = 0;
}

void terminal_initialize(void) {
    // Initialize all screens
    for (size_t i = 0; i < NUM_SCREENS; i++) {
        screens[i].row = 0;
        screens[i].column = 0;
        screens[i].color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        // Clear each screen
        for (size_t y = 0; y < VGA_HEIGHT; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                const size_t index = y * VGA_WIDTH + x;
                screens[i].buffer[index] = vga_entry(' ', screens[i].color);
            }
        }
    }
    
    // Set initial screen
    current_screen = 0;
    terminal_clear();
}

void switch_screen(size_t screen_num) {
    if (screen_num >= NUM_SCREENS) return;
    
    // Save current screen state to VGA buffer
    screen_t* current = &screens[current_screen];
    for (size_t i = 0; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        current->buffer[i] = vga_buffer[i];
    }
    
    // Switch to new screen
    current_screen = screen_num;
    screen_t* new = &screens[current_screen];
    
    // Copy new screen to VGA buffer
    for (size_t i = 0; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        vga_buffer[i] = new->buffer[i];
    }
    
    // Update cursor position
    terminal_setcursor(new->column, new->row);
}

void terminal_putchar(char c) {
    screen_t* screen = &screens[current_screen];
    
    if (c == '\n') {
        screen->column = 0;
        if (++screen->row == VGA_HEIGHT) {
            terminal_scroll();
        }
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
        vga_buffer[index] = screen->buffer[index];
        return;
    }

    const size_t index = screen->row * VGA_WIDTH + screen->column;
    screen->buffer[index] = vga_entry(c, screen->color);
    vga_buffer[index] = screen->buffer[index];

    if (++screen->column == VGA_WIDTH) {
        screen->column = 0;
        if (++screen->row == VGA_HEIGHT) {
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
    screen_t* screen = &screens[current_screen];
    if (x >= VGA_WIDTH) x = VGA_WIDTH - 1;
    if (y >= VGA_HEIGHT) y = VGA_HEIGHT - 1;
    screen->column = x;
    screen->row = y;
} 