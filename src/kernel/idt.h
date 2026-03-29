// idt.h: IDT structure and function declarations
// Provides IDT entry structure and IDT init prototypes
#ifndef KERNEL_IDT_H
#define KERNEL_IDT_H

#include "types.h"

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void idt_init(void);

#endif
