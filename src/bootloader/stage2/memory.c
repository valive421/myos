#include "memory.h"

/*
 * Small real-mode memory helpers shared by stage2 modules.
 */

void far* MEM_FarPtr(uint16_t segment, uint16_t offset)
{
    return (void far*)((((uint32_t)segment) << 16) | offset);
}

bool MEM_CanAccessSegmentRange(uint16_t offset, uint16_t length)
{
    return ((uint32_t)offset + (uint32_t)length) <= 0x10000ul;
}
