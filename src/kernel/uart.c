#include <stdint.h>
#include "io.h"

#define COM1 0x3F8

static void uart_putc(char c) {
    // Wait until transmitter is ready
    while ((inb(COM1 + 5) & 0x20) == 0);
    outb(COM1, c);
}

void uart_puts(const char* str) {
    while (*str) {
        uart_putc(*str++);
    }
}

void uart_init(void) {
    // Disable interrupts
    outb(COM1 + 1, 0x00);
    
    // Enable DLAB
    outb(COM1 + 3, 0x80);
    
    // Set divisor (115200 baud)
    outb(COM1 + 0, 0x01);
    outb(COM1 + 1, 0x00);
    
    // 8 bits, no parity, one stop bit
    outb(COM1 + 3, 0x03);
    
    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1 + 2, 0xC7);
    
    // IRQs enabled, RTS/DSR set
    outb(COM1 + 4, 0x0B);
} 