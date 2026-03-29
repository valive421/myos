#pragma once

#include "types.h"

void vga_init(void);
void vga_clear(void);
void vga_backspace(void);
void vga_write_at(uint8_t row, uint8_t col, char c);
void putc(char c);
void puts(const char* s);
void vga_puthex8(uint8_t value);
void vga_puthex32(uint32_t value);
