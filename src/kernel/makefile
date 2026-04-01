BUILD_DIR?=build/
ASM?=nasm
CC?=gcc
LD?=ld

ASMFLAGS?=-f elf32
CFLAGS?=-m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -O2 -Wall -Wextra
LDFLAGS?=-m elf_i386 -nostdlib -T linker.ld

SOURCES_ASM=main.asm interrupts.asm task_switch.asm
SOURCES_ASM+=user_mode.asm
SOURCES_C=kernel.c io.c boot.c vga.c gdt.c idt.c pic.c interrupts.c timer.c keyboard.c shell.c serial.c stdio.c memory.c vfs.c task.c syscall.c ata.c fat12.c

USER_DIR=../user
USER_SOURCES_C=$(USER_DIR)/user_test.c $(USER_DIR)/notepad.c
USER_OBJECTS_C=$(patsubst $(USER_DIR)/%.c,$(BUILD_DIR)/user/c/%.obj,$(USER_SOURCES_C))

USER_CFLAGS?=-m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -O2 -Wall -Wextra

OBJECTS_ASM=$(patsubst %.asm,$(BUILD_DIR)/kernel/asm/%.obj,$(SOURCES_ASM))
OBJECTS_C=$(patsubst %.c,$(BUILD_DIR)/kernel/c/%.obj,$(SOURCES_C))
OBJECTS=$(OBJECTS_ASM) $(OBJECTS_C) $(USER_OBJECTS_C)

.PHONY: all kernel clean

all: kernel

kernel: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/kernel.elf $(OBJECTS)
	objcopy -O binary $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel/asm/%.obj: %.asm
	mkdir -p $(BUILD_DIR)/kernel/asm
	$(ASM) $(ASMFLAGS) -o $@ $<

$(BUILD_DIR)/kernel/c/%.obj: %.c
	mkdir -p $(BUILD_DIR)/kernel/c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/user/c/%.obj: $(USER_DIR)/%.c
	mkdir -p $(BUILD_DIR)/user/c
	cd $(USER_DIR) && $(CC) $(USER_CFLAGS) -Iinclude -c -o $(abspath $@) $(notdir $<)

clean:
	rm -f $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/kernel.elf
	rm -rf $(BUILD_DIR)/kernel
	rm -rf $(BUILD_DIR)/user