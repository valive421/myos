#pragma once
#include "stdint.h"

void _cdecl x86_Video_WriteCharTeletype(char c, uint8_t page);
uint8_t _cdecl x86_inb(uint16_t port);
void _cdecl x86_outb(uint16_t port, uint8_t value);
