#ifndef UART_H
#define UART_H

#include <stdint.h>

// UART ports
#define UART_PORT 0x3F8

// UART functions
void uart_init(void);
void uart_write_char(char c);
void uart_write_string(const char* str);
void uart_write_hex(uint32_t value);

#endif 