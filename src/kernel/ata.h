#pragma once

#include "types.h"

// Minimal ATA PIO driver (primary channel, master drive, LBA28).
// Used for a persistent disk image in QEMU.

// Returns 1 if an ATA device is detected and usable, else 0.
int ata_init(void);

// Returns 1 if ata_init() succeeded at least once.
int ata_present(void);

// Read/write `count` 512-byte sectors starting at `lba`.
// Returns 1 on success, 0 on failure.
int ata_read_sectors(uint32_t lba, uint8_t count, void* out);
int ata_write_sectors(uint32_t lba, uint8_t count, const void* in);
