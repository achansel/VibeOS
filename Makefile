# Toolchain
CC = clang
ASM = nasm
LD = ld

# Flags
CFLAGS = -m32 \
         -fno-builtin \
         -fno-exceptions \
         -fno-stack-protector \
         -nostdlib \
         -nodefaultlibs \
         -Wall \
         -Wextra \
         -ffreestanding \
         -O2 \
         --target=i386-pc-none-elf

ASFLAGS = -f elf32

LDFLAGS = -m elf_i386 \
          -T src/linker.ld \
          -nostdlib

# Directories
OBJ_DIR = obj
ISO_DIR = iso
BOOT_DIR = $(ISO_DIR)/boot
GRUB_DIR = $(BOOT_DIR)/grub

# Files
KERNEL = kernel.bin
ISO = kernel.iso

# Default target
all: $(ISO)

# Create object directories
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)/kernel
	mkdir -p $(OBJ_DIR)/boot

# Compile kernel
$(OBJ_DIR)/kernel/kernel.o: src/kernel/kernel.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile terminal
$(OBJ_DIR)/kernel/terminal.o: src/kernel/terminal.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile keyboard
$(OBJ_DIR)/kernel/keyboard.o: src/kernel/keyboard.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile UART
$(OBJ_DIR)/kernel/uart.o: src/kernel/uart.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile GDT
$(OBJ_DIR)/kernel/gdt.o: src/kernel/gdt.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile Stack
$(OBJ_DIR)/kernel/stack.o: src/kernel/stack.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile GDT assembly
$(OBJ_DIR)/kernel/gdt_asm.o: src/kernel/gdt_asm.s | $(OBJ_DIR)
	$(ASM) $(ASFLAGS) $< -o $@

# Compile Stack assembly
$(OBJ_DIR)/kernel/stack_asm.o: src/kernel/stack_asm.s | $(OBJ_DIR)
	$(ASM) $(ASFLAGS) $< -o $@

# Compile bootloader
$(OBJ_DIR)/boot/boot.o: src/boot/boot.asm | $(OBJ_DIR)
	$(ASM) $(ASFLAGS) $< -o $@

# Link kernel
$(KERNEL): $(OBJ_DIR)/kernel/kernel.o $(OBJ_DIR)/kernel/terminal.o $(OBJ_DIR)/kernel/keyboard.o $(OBJ_DIR)/kernel/uart.o $(OBJ_DIR)/kernel/gdt.o $(OBJ_DIR)/kernel/stack.o $(OBJ_DIR)/kernel/gdt_asm.o $(OBJ_DIR)/kernel/stack_asm.o $(OBJ_DIR)/boot/boot.o
	$(LD) $(LDFLAGS) -o $@ $^

# Create ISO directory structure
$(ISO_DIR):
	mkdir -p $(BOOT_DIR)
	mkdir -p $(GRUB_DIR)

# Copy files to ISO directory
$(BOOT_DIR)/$(KERNEL): $(KERNEL) | $(ISO_DIR)
	cp $< $@

$(GRUB_DIR)/grub.cfg: grub.cfg | $(ISO_DIR)
	cp $< $@

# Create ISO
$(ISO): $(BOOT_DIR)/$(KERNEL) $(GRUB_DIR)/grub.cfg
	grub2-mkrescue -o $@ $(ISO_DIR)

# Clean build files
clean:
	rm -rf $(OBJ_DIR) $(ISO_DIR) $(KERNEL) $(ISO)

# Run the kernel in QEMU
run: $(ISO)
	qemu-system-i386 \
		-m 1G \
		-cdrom $(ISO) \
		-serial stdio

.PHONY: all clean run 