#include "memory.h"

/*
 * Small real-mode memory helpers shared by stage2 modules.
 */

bool MEM_CanAccessSegmentRange(uint16_t offset, uint16_t length)
{
    return ((uint32_t)offset + (uint32_t)length) <= 0x10000ul;
}
