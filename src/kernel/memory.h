#pragma once
// memory.h: Kernel heap allocator declarations
// Provides Phase-2 memory management APIs for dynamic allocations and tests

#include "types.h"

void memory_init(void);
void* kmalloc(uint32_t size);
void* kcalloc(uint32_t count, uint32_t size);
void kfree(void* ptr);

uint32_t memory_heap_total(void);
uint32_t memory_heap_used(void);
uint32_t memory_heap_free(void);

int memory_run_self_test(void);
