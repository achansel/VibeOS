#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "io.h"
#include "uart.h"
#include "terminal.h"
#include "keyboard.h"


// Command buffer
static char command_buffer[256];
static size_t command_length = 0;

// Custom strlen implementation
static size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
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
        terminal_writestring("Press 1 or 2 to switch screens\n");
    }
    
    command_length = 0;
}

void kernel_main(uint32_t magic __attribute__((unused)), void* mb_info __attribute__((unused))) {
    // Initialize UART first for debugging
    uart_init();
    uart_write_string("UART initialized\n");
    
    // Initialize keyboard
    keyboard_init();
    uart_write_string("Keyboard initialized\n");
    
    // Initialize VGA
    terminal_initialize();
    uart_write_string("Terminal initialized\n");
    
    // Set terminal color
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    uart_write_string("Terminal color set\n");
    
    // Print welcome message
    terminal_writestring("Hello from kernel_main()\n");
    terminal_writestring("--- this message was sent from the std vga device ---\n");
    terminal_writestring("and thats a great boot log\n");
    terminal_writestring("Press 1 or 2 to switch between screens\n");
    uart_write_string("Welcome message printed\n");
    
    // Initialize command buffer
    command_length = 0;
    uart_write_string("Command buffer initialized\n");
    
    // Show prompt
    terminal_writestring("> ");
    uart_write_string("Prompt shown\n");
    
    uart_write_string("Entering main loop\n");
    
    while (1) {
        if (keyboard_is_key_pressed()) {
            uint8_t scancode = keyboard_get_scancode();
            
            if (!keyboard_is_released(scancode)) {  // Only process key press, not release
                char ascii = keyboard_scancode_to_ascii(scancode);
                
                // Handle screen switching
                if (ascii == '1') {
                    switch_screen(0);
                    terminal_writestring("\nSwitched to screen 1\n> ");
                    continue;
                } else if (ascii == '2') {
                    switch_screen(1);
                    terminal_writestring("\nSwitched to screen 2\n> ");
                    continue;
                }
                
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