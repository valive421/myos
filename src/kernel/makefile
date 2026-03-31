BUILD_DIR?=build/
ASM?=nasm
CC?=gcc
LD?=ld

ASMFLAGS?=-f elf32
CFLAGS?=-m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -O2 -Wall -Wextra
LDFLAGS?=-m elf_i386 -nostdlib -T linker.ld

SOURCES_ASM=main.asm interrupts.asm task_switch.asm
SOURCES_C=kernel.c io.c boot.c vga.c gdt.c idt.c pic.c interrupts.c timer.c keyboard.c shell.c serial.c stdio.c memory.c task.c

OBJECTS_ASM=$(patsubst %.asm,$(BUILD_DIR)/kernel/asm/%.obj,$(SOURCES_ASM))
OBJECTS_C=$(patsubst %.c,$(BUILD_DIR)/kernel/c/%.obj,$(SOURCES_C))
OBJECTS=$(OBJECTS_ASM) $(OBJECTS_C)

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

clean:
	rm -f $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/kernel.elf
	rm -rf $(BUILD_DIR)/kernel