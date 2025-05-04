#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "io.h"
#include "uart.h"
#include "terminal.h"
#include "keyboard.h"
#include "gdt.h"
#include "stack.h"


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

// Custom strcmp implementation
static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void handle_command(void) {
    command_buffer[command_length] = '\0';
    
    if (strcmp(command_buffer, "clear") == 0) {
        terminal_clear();
    } else if (strcmp(command_buffer, "stack") == 0) {
        print_kernel_stack();
    } else if (strcmp(command_buffer, "gdt") == 0) {
        verify_gdt();
    } else if (strcmp(command_buffer, "poweroff") == 0) {
        // Try ACPI shutdown first
        outw(0x604, 0x2000);  // QEMU poweroff
        // If that fails, try Bochs shutdown
        outw(0xB004, 0x2000); // Bochs poweroff
        // If that fails, try VirtualBox shutdown
        outw(0x4004, 0x3400); // VirtualBox poweroff
    } else {
        terminal_writestring("\nUnknown command. Available commands:\n");
        terminal_writestring("clear     - Clear the screen\n");
        terminal_writestring("stack     - Print kernel stack trace\n");
        terminal_writestring("gdt       - Print GDT contents\n");
        terminal_writestring("poweroff  - Shut down the system\n");
    }
    
    command_length = 0;
}

void kernel_main(uint32_t magic __attribute__((unused)), void* mb_info __attribute__((unused))) {
    // Initialize UART first for debugging
    uart_init();
    uart_write_string("UART initialized\n");
    
    // Initialize GDT
    init_gdt();
    uart_write_string("GDT initialized\n");
    
    // Initialize keyboard
    keyboard_init();
    uart_write_string("Keyboard initialized\n");
    
    // Initialize VGA
    terminal_initialize();
    uart_write_string("Terminal initialized\n");
    
    // Enable cursor
    terminal_enable_cursor();
    
    // Set terminal color
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    uart_write_string("Terminal color set\n");
    
    // Print welcome message
    terminal_writestring("Hello from kernel_main()\n");
    terminal_writestring("--- this message was sent from the std vga device ---\n");
    terminal_writestring("and thats a great boot log\n");
    terminal_writestring("Press F1-F12 to switch between screens\n");
    terminal_writestring("Type 'stack' to print kernel stack trace\n");
    terminal_writestring("Type 'gdt' to print GDT contents\n");
    terminal_writestring("Type 'poweroff' to shut down the system\n");
    uart_write_string("Welcome message printed\n");
    
    // Initialize command
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
                uint8_t keycode = keyboard_get_keycode(scancode);
                
                // Handle screen switching with F1-F12
                if (keycode >= KEY_F1 && keycode <= KEY_F10) {
                    uint8_t screen_num = keycode - KEY_F1;
                    if (screen_num < NUM_SCREENS) {
                        terminal_switch_screen(screen_num);
                    }
                    continue;
                }
                // Handle F11 and F12 separately since they have different scancodes
                else if (keycode == KEY_F11) {
                    if (10 < NUM_SCREENS) {
                        terminal_switch_screen(10);
                    }
                    continue;
                }
                else if (keycode == KEY_F12) {
                    if (11 < NUM_SCREENS) {
                        terminal_switch_screen(11);
                    }
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