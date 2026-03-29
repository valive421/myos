#pragma once

#include "types.h"

extern uint32_t kernel_base_phys;

uint32_t linear_addr(const void* p);
uint32_t kernel_linear_addr(const void* p);
void boot_enable_a20_fast(void);
