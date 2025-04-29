#include "keyboard.h"
#include "io.h"

// Keyboard scancode to ASCII mapping
static const char scancode_to_ascii[] = {
    '\0', 0x1b, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    '\0', 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    '\0', '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    '\0', '*', '\0', ' ', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '{', '\0', '\0', '[', '\0', ']', '\0', '\0', '}'
};

void keyboard_init(void) {
    // Nothing to initialize for now
}

bool keyboard_is_key_pressed(void) {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    return (status & 1) != 0;  // Check if output buffer is full
}

uint8_t keyboard_get_scancode(void) {
    return inb(KEYBOARD_DATA_PORT);
}

bool keyboard_is_released(uint8_t scancode) {
    return (scancode & 0x80) != 0;  // Check if the highest bit is set
}

uint8_t keyboard_get_keycode(uint8_t scancode) {
    return scancode & 0x7F;  // Remove the release bit
}

char keyboard_scancode_to_ascii(uint8_t scancode) {
    uint8_t keycode = keyboard_get_keycode(scancode);
    if (keycode < sizeof(scancode_to_ascii)) {
        return scancode_to_ascii[keycode];
    }
    return '\0';
} 