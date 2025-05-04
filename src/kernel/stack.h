#ifndef STACK_H
#define STACK_H

#include <stdint.h>

// Function to print the current kernel stack
void print_kernel_stack(void);

// Function to get the current stack pointer
uint32_t get_stack_pointer(void);

// Function to get the current base pointer
uint32_t get_base_pointer(void);

#endif // STACK_H 