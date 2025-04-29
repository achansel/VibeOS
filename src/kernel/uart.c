#include <stdint.h>
#include "uart.h"
#include "io.h"

#define COM1 0x3F8

void uart_write_char(char c) {
    // Wait for transmit buffer to be empty
    while ((inb(UART_PORT + 5) & 0x20) == 0);
    outb(UART_PORT, c);
}

void uart_write_string(const char* str) {
    while (*str) {
        uart_write_char(*str++);
    }
}

void uart_init(void) {
    // Disable interrupts
    outb(UART_PORT + 1, 0x00);
    
    // Enable DLAB
    outb(UART_PORT + 3, 0x80);
    
    // Set divisor (115200 baud)
    outb(UART_PORT + 0, 0x01);
    outb(UART_PORT + 1, 0x00);
    
    // 8 bits, no parity, one stop bit
    outb(UART_PORT + 3, 0x03);
    
    // Enable FIFO, clear them, with 14-byte threshold
    outb(UART_PORT + 2, 0xC7);
    
    // Enable interrupts
    outb(UART_PORT + 4, 0x0B);
}

void uart_write_hex(uint32_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    uart_write_string("0x");
    
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        uart_write_char(hex_chars[nibble]);
    }
} 