BUILD_DIR?=build/
ASM?=nasm
CC16?=/usr/bin/watcom/binl/wcc
LD16?=/usr/bin/watcom/binl/wlink

SOURCES_C=$(wildcard *.c)
SOURCES_ASM=$(wildcard *.asm)
OBJECTS_C=$(patsubst %.c, $(BUILD_DIR)/kernel/c/%.obj, $(SOURCES_C))
OBJECTS_ASM=$(patsubst %.asm, $(BUILD_DIR)/kernel/asm/%.obj, $(SOURCES_ASM))

.PHONY: all kernel clean

all: kernel

kernel: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	@echo "Linking kernel..."
	$(LD16) NAME $(BUILD_DIR)/kernel.bin FILE \{$(OBJECTS_ASM) $(OBJECTS_C)\} OPTION MAP=$(BUILD_DIR)/kernel.map @linker.lnk

$(BUILD_DIR)/kernel/c/%.obj: %.c
	@mkdir -p $(dir $@)
	$(CC16) -4 -d3 -s -wx -ms -zl -zq -za99 -fo=$@ $<

$(BUILD_DIR)/kernel/asm/%.obj: %.asm
	@mkdir -p $(dir $@)
	$(ASM) $< -f obj -o $@

clean:
	rm -f $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/kernel.map
	rm -rf $(BUILD_DIR)/kernel/