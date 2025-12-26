#!/bin/bash

# Simple test script to verify code structure

echo "====================================="
echo "MyOS Structure Verification"
echo "====================================="
echo ""

# Check for required source files
echo "Checking kernel source files..."
FILES=(
    "src/kernel/kmain.c"
    "src/kernel/entry.asm"
    "src/kernel/network.c"
    "src/kernel/network.h"
    "src/kernel/ne2000.c"
    "src/kernel/ne2000.h"
    "src/kernel/http.c"
    "src/kernel/http.h"
    "src/kernel/browser.c"
    "src/kernel/browser.h"
    "src/kernel/stdio.c"
    "src/kernel/stdio.h"
    "src/kernel/string.c"
    "src/kernel/string.h"
    "src/kernel/x86.asm"
    "src/kernel/x86.h"
)

missing=0
for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "  ✓ $file"
    else
        echo "  ✗ $file (MISSING)"
        missing=$((missing + 1))
    fi
done

echo ""
echo "Checking bootloader files..."
BOOT_FILES=(
    "src/bootloader/stage1/boot.asm"
    "src/bootloader/stage2/main.c"
)

for file in "${BOOT_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "  ✓ $file"
    else
        echo "  ✗ $file (MISSING)"
        missing=$((missing + 1))
    fi
done

echo ""
echo "Checking build configuration..."
if [ -f "makefile" ]; then
    echo "  ✓ Main makefile exists"
else
    echo "  ✗ Main makefile missing"
    missing=$((missing + 1))
fi

if [ -f "src/kernel/makefile" ]; then
    echo "  ✓ Kernel makefile exists"
else
    echo "  ✗ Kernel makefile missing"
    missing=$((missing + 1))
fi

echo ""
echo "====================================="
if [ $missing -eq 0 ]; then
    echo "✓ All files present!"
    echo "====================================="
    exit 0
else
    echo "✗ $missing file(s) missing!"
    echo "====================================="
    exit 1
fi
