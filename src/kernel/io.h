#pragma once

#include "types.h"

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void io_wait(void);

void outw(uint16_t port, uint16_t value);
uint16_t inw(uint16_t port);

// Word-string I/O helpers (count is in 16-bit words).
void insw(uint16_t port, void* dst, uint32_t count);
void outsw(uint16_t port, const void* src, uint32_t count);
