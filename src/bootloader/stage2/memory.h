#pragma once

#include "stdint.h"

/*
 * Checks whether [offset, offset + length) fits in one 64 KiB segment.
 */
bool MEM_CanAccessSegmentRange(uint16_t offset, uint16_t length);
