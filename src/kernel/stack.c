#include "stack.h"
#include "terminal.h"
#include <stdint.h>

// Assembly functions to get stack and base pointers
extern uint32_t get_esp(void);
extern uint32_t get_ebp(void);

uint32_t get_stack_pointer(void) {
    return get_esp();
}

uint32_t get_base_pointer(void) {
    return get_ebp();
}

void print_kernel_stack(void) {
    uint32_t *ebp = (uint32_t *)get_base_pointer();
    uint32_t *esp = (uint32_t *)get_stack_pointer();
    
    terminal_writestring("Kernel Stack Trace:\n");
    terminal_writestring("==================\n");
    
    // Print stack frames
    while (ebp > esp) {
        terminal_writestring("EBP: 0x");
        terminal_writehex((uint32_t)ebp);
        terminal_writestring("  EIP: 0x");
        terminal_writehex(ebp[1]);
        terminal_writestring("\n");
        
        // Move to previous frame
        ebp = (uint32_t *)*ebp;
    }
    
    terminal_writestring("==================\n");
} 