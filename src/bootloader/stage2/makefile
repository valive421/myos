BUILD_DIR?=build/
ASM?=nasm
ASMFLAGS?=-f elf32
CC16?=gcc
CFLAGS16?=-m16 -march=i386 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-builtin -O2 -Wall -Wextra
LD16?=ld
LDFLAGS16?=-m elf_i386 -nostdlib -T linker.ld --oformat binary -Map $(BUILD_DIR)/stage2.map

SOURCES_C=$(wildcard *.c)
SOURCES_ASM=$(wildcard *.asm)
OBJECTS_C=$(patsubst %.c, $(BUILD_DIR)/stage2/c/%.obj, $(SOURCES_C))
OBJECTS_ASM=$(patsubst %.asm, $(BUILD_DIR)/stage2/asm/%.obj, $(SOURCES_ASM))

.PHONY: all stage2 clean always

all: stage2

stage2: $(BUILD_DIR)/stage2.bin

$(BUILD_DIR)/stage2.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	$(LD16) $(LDFLAGS16) -o $(BUILD_DIR)/stage2.bin $(OBJECTS_ASM) $(OBJECTS_C)

$(BUILD_DIR)/stage2/c/%.obj: %.c always
	$(CC16) $(CFLAGS16) -c -o $@ $<

$(BUILD_DIR)/stage2/asm/%.obj: %.asm always
	$(ASM) $(ASMFLAGS) -o $@ $<

always:
	mkdir -p $(BUILD_DIR)/stage2/c
	mkdir -p $(BUILD_DIR)/stage2/asm

clean:
	rm -f $(BUILD_DIR)/stage2.bin