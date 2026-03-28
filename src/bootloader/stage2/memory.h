#pragma once

#include "stdint.h"

/*
 * Creates a real-mode far pointer from segment:offset.
 */
void far* MEM_FarPtr(uint16_t segment, uint16_t offset);

/*
 * Checks whether [offset, offset + length) fits in one 64 KiB segment.
 */
bool MEM_CanAccessSegmentRange(uint16_t offset, uint16_t length);
