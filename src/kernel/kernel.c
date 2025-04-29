#include <stdint.h>
#include <stddef.h>
#include "io.h"
#include "uart.h"

// VGA text mode constants
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ADDRESS 0xB8000

// Boolean type
typedef enum { false = 0, true = 1 } bool;

// VGA color constants
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

// Keyboard scancode to ASCII mapping
static const char scancode_to_ascii[] = {
    '\0', 0x1b, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    '\0', 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    '\0', '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    '\0', '*', '\0', ' ', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '{', '\0', '\0', '[', '\0', ']', '\0', '\0', '}'
};

// VGA buffer must be in the .vga_buffer section
static uint16_t* vga_buffer __attribute__((section(".vga_buffer"))) = (uint16_t*)VGA_ADDRESS;

// Terminal state
static size_t terminal_row = 0;
static size_t terminal_column = 0;
static uint8_t terminal_color = 0;

// Command buffer
static char command_buffer[256];
static size_t command_length = 0;

// Add keyboard port
#define KEYBOARD_PORT 0x60
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_CMD_PORT 0x64

// Add keyboard status port
#define KEYBOARD_STATUS_PORT 0x64

// Add keyboard scancodes
enum key_scancodes {
    KEY_A = 0x1E,
    KEY_B = 0x30,
    KEY_C = 0x2E,
    KEY_D = 0x20,
    KEY_E = 0x12,
    KEY_F = 0x21,
    KEY_G = 0x22,
    KEY_H = 0x23,
    KEY_I = 0x17,
    KEY_J = 0x24,
    KEY_K = 0x25,
    KEY_L = 0x26,
    KEY_M = 0x32,
    KEY_N = 0x31,
    KEY_O = 0x18,
    KEY_P = 0x19,
    KEY_Q = 0x10,
    KEY_R = 0x13,
    KEY_S = 0x1F,
    KEY_T = 0x14,
    KEY_U = 0x16,
    KEY_V = 0x2F,
    KEY_W = 0x11,
    KEY_X = 0x2D,
    KEY_Y = 0x15,
    KEY_Z = 0x2C,
    KEY_ENTER = 0x1C,
    KEY_BACKSPACE = 0x0E,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t)uc | (uint16_t)color << 8;
}

void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

void terminal_scroll(void) {
    // Move all lines up by one
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t current = y * VGA_WIDTH + x;
            const size_t next = (y + 1) * VGA_WIDTH + x;
            vga_buffer[current] = vga_buffer[next];
        }
    }
    
    // Clear the last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        vga_buffer[index] = vga_entry(' ', terminal_color);
    }
    
    terminal_row = VGA_HEIGHT - 1;
}

void terminal_clear(void) {
    uart_puts("DEBUG: Starting terminal_clear\n");
    
    // First, verify we can write to the VGA buffer
    uart_puts("DEBUG: Testing VGA buffer write\n");
    uint16_t test_value = vga_entry('X', terminal_color);
    volatile uint16_t* test_ptr = (volatile uint16_t*)vga_buffer;
    test_ptr[0] = test_value;
    
    // Verify the write
    if (test_ptr[0] != test_value) {
        uart_puts("ERROR: VGA buffer not writable\n");
        asm volatile("hlt");
    }
    
    uart_puts("DEBUG: VGA buffer verified\n");
    
    // Clear the screen with bounds checking
    uart_puts("DEBUG: Starting screen clear\n");
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            if (index < VGA_WIDTH * VGA_HEIGHT) {  // Safety check
                test_ptr[index] = vga_entry(' ', terminal_color);
            } else {
                uart_puts("ERROR: VGA buffer index out of bounds\n");
                asm volatile("hlt");
            }
        }
    }
    
    uart_puts("DEBUG: Screen cleared\n");
    
    terminal_row = 0;
    terminal_column = 0;
}

void terminal_initialize(void) {
    uart_puts("DEBUG: Starting terminal_initialize\n");
    
    // Initialize terminal state
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    uart_puts("DEBUG: Terminal state initialized\n");
    
    // Clear the screen
    terminal_clear();
    
    uart_puts("DEBUG: Terminal initialized successfully\n");
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
        }
        return;
    }
    
    if (c == '\t') {
        terminal_column = (terminal_column + 8) & ~(8 - 1);
        if (terminal_column >= VGA_WIDTH) {
            terminal_column = 0;
            if (++terminal_row == VGA_HEIGHT) {
                terminal_scroll();
            }
        }
        return;
    }
    
    if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
        } else if (terminal_row > 0) {
            terminal_row--;
            terminal_column = VGA_WIDTH - 1;
        }
        const size_t index = terminal_row * VGA_WIDTH + terminal_column;
        vga_buffer[index] = vga_entry(' ', terminal_color);
        return;
    }

    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    vga_buffer[index] = vga_entry(c, terminal_color);

    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
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
    terminal_column = x;
    terminal_row = y;
}

void handle_command(void) {
    command_buffer[command_length] = '\0';
    
    if (strlen(command_buffer) == 5 && 
        command_buffer[0] == 'c' &&
        command_buffer[1] == 'l' &&
        command_buffer[2] == 'e' &&
        command_buffer[3] == 'a' &&
        command_buffer[4] == 'r') {
        terminal_clear();
    } else {
        terminal_writestring("\nUnknown command. Available commands:\n");
        terminal_writestring("clear - Clear the screen\n");
    }
    
    command_length = 0;
}

void kernel_main(uint32_t magic __attribute__((unused)), void* mb_info __attribute__((unused))) {
    // Initialize UART first for debugging
    uart_init();
    uart_puts("DEBUG: Starting kernel_main\n");
    
    // Initialize VGA
    uart_puts("DEBUG: Starting VGA initialization\n");
    
    // Verify VGA buffer address
    uart_puts("DEBUG: Verifying VGA buffer address\n");
    if ((uint32_t)vga_buffer != VGA_ADDRESS) {
        uart_puts("ERROR: VGA buffer not at correct address\n");
        asm volatile("hlt");
    }
    
    terminal_initialize();
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    
    uart_puts("DEBUG: VGA initialized\n");
    
    // Print welcome message
    uart_puts("DEBUG: Printing welcome message\n");
    terminal_writestring("Hello from kernel_main()\n");
    terminal_writestring("--- this message was sent from the std vga device ---\n");
    terminal_writestring("and thats a great boot log\n");
    
    // Initialize command buffer
    uart_puts("DEBUG: Starting command buffer initialization\n");
    
    // Initialize command length
    uart_puts("DEBUG: Setting command length to 0\n");
    command_length = 0;
    uart_puts("DEBUG: Command length initialized\n");
    
    // Clear command buffer with bounds checking
    uart_puts("DEBUG: Starting command buffer clear\n");
    volatile char* cmd_buf = (volatile char*)command_buffer;
    for (size_t i = 0; i < sizeof(command_buffer); i++) {
        if (i < sizeof(command_buffer)) {
            cmd_buf[i] = '\0';
        } else {
            uart_puts("ERROR: Command buffer index out of bounds\n");
            asm volatile("hlt");
        }
    }
    uart_puts("DEBUG: Command buffer cleared\n");
    
    // Show prompt
    uart_puts("DEBUG: Showing prompt\n");
    terminal_writestring("> ");
    uart_puts("DEBUG: Prompt shown\n");
    
    uart_puts("DEBUG: Entering main loop\n");
    
    while (1) {
        // Show cursor
        size_t cursor_index = terminal_row * VGA_WIDTH + terminal_column;
        vga_buffer[cursor_index] = vga_entry('_', terminal_color);
        
        // Check keyboard status
        uint8_t status = inb(0x64);
        if (status & 1) {  // Only if output buffer is full
            // Read scancode
            uint8_t raw_data = inb(0x60);
            bool released = (raw_data & 0x80) >> 7;
            
            if (!released) {  // Only process key press, not release
                // Convert scancode to ASCII
                uint8_t scancode = raw_data & 0x7F;
                if (scancode < sizeof(scancode_to_ascii)) {
                    char ascii = scancode_to_ascii[scancode];
                    
                    // Hide cursor before processing input
                    vga_buffer[cursor_index] = vga_entry(' ', terminal_color);
                    
                    // Handle backspace
                    if (ascii == '\b' && command_length > 0) {
                        command_length--;
                        terminal_putchar('\b');
                    }
                    // Handle enter
                    else if (ascii == '\n') {
                        terminal_putchar('\n');
                        handle_command();
                        terminal_writestring("> ");
                    }
                    // Handle regular characters
                    else if ((ascii >= 'a' && ascii <= 'z') || ascii == ' ') {
                        if (command_length < sizeof(command_buffer) - 1) {
                            command_buffer[command_length++] = ascii;
                            terminal_putchar(ascii);
                        }
                    }
                }
            }
        }
    }
} 