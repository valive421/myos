ASM = nasm

SRC_DIR = src
BUILD_DIR = build

.PHONY: all run clean

all: $(BUILD_DIR)/main_floppy.img

#
# floppy image build
#
$(BUILD_DIR)/main_floppy.img: $(BUILD_DIR)/bootloader.bin $(BUILD_DIR)/main.bin
	dd if=/dev/zero of=$(BUILD_DIR)/main_floppy.img bs=512 count=2880
	mkfs.fat -F 12 -n "NBOS" $(BUILD_DIR)/main_floppy.img
	dd if=$(BUILD_DIR)/bootloader.bin of=$(BUILD_DIR)/main_floppy.img conv=notrunc
	mcopy -i $(BUILD_DIR)/main_floppy.img $(BUILD_DIR)/main.bin ::kernel.bin
#
# bootloader build
#
bootloader : $(BUILD_DIR)/bootloader.bin

${BUILD_DIR}/bootloader.bin: always
	$(ASM) $(SRC_DIR)/bootloader/boot.asm -f bin -o $(BUILD_DIR)/bootloader.bin



#
# kernel build
# 
kernel: $(BUILD_DIR)/bootloader.bin
$(BUILD_DIR)/main.bin: always
	$(ASM) $(SRC_DIR)/kernel/main.asm -f bin -o $(BUILD_DIR)/main.bin

#
#always
#
always:
	@mkdir -p $(BUILD_DIR)

#
#clear
#


#
#to build and run the floppy image in QEMU
#
run: $(BUILD_DIR)/main_floppy.img
	qemu-system-i386 -fda $(BUILD_DIR)/main_floppy.img

clean:
	rm -f $(BUILD_DIR)/*.bin $(BUILD_DIR)/*.img
