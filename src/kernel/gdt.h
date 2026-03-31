#ifndef KERNEL_GDT_H
#define KERNEL_GDT_H

#include "types.h"

void gdt_init(void);
void gdt_set_kernel_stack(uint32_t esp0);

#endif
