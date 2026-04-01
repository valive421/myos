#!/bin/sh
set -e

# Persistent disk image (separate from the boot floppy).
# Files written via the kernel VFS are synced here and survive reboots.
PERSIST_IMG=build/persist.img
if [ ! -f "$PERSIST_IMG" ]; then
	dd if=/dev/zero of="$PERSIST_IMG" bs=1M count=16
	mkfs.fat -F 12 -n "NBOSDATA" "$PERSIST_IMG"
fi

qemu-system-i386 \
	-drive if=floppy,format=raw,file=build/main_floppy.img \
	-drive if=ide,format=raw,file="$PERSIST_IMG" \
	-boot order=a \
	-serial stdio \
	-monitor none