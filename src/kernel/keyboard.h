#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

// Keyboard ports
#define KEYBOARD_PORT 0x60
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_CMD_PORT 0x64
#define KEYBOARD_STATUS_PORT 0x64

// Keyboard scancodes
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
    KEY_F1 = 0x3B,
    KEY_F2 = 0x3C,
    KEY_F3 = 0x3D,
    KEY_F4 = 0x3E,
    KEY_F5 = 0x3F,
    KEY_F6 = 0x40,
    KEY_F7 = 0x41,
    KEY_F8 = 0x42,
    KEY_F9 = 0x43,
    KEY_F10 = 0x44,
    KEY_F11 = 0x57,
    KEY_F12 = 0x58,
};

// Keyboard functions
void keyboard_init(void);
bool keyboard_is_key_pressed(void);
uint8_t keyboard_get_scancode(void);
bool keyboard_is_released(uint8_t scancode);
uint8_t keyboard_get_keycode(uint8_t scancode);
char keyboard_scancode_to_ascii(uint8_t scancode);

#endif 