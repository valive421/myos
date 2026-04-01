#pragma once

#include "types.h"

// Kernel FAT12 driver (root directory only, 8.3 names, no LFN).
// Backed by the ATA PIO driver (primary master) and treats LBA 0 as the FAT volume.

// Mount FAT12 volume and cache FAT + root directory.
// Returns 1 on success, 0 on failure.
int fat12_init(void);

// Returns 1 if FAT12 volume is mounted and cached.
int fat12_ready(void);

// Returns 1 if `name` can be represented as an 8.3 FAT name (no paths, no LFN).
int fat12_is_valid_name(const char* name);

typedef struct
{
    char name[32];
    uint32_t size;
} fat12_file_info_t;

// List up to `max` files in the FAT12 root directory.
// Returns 1 on success.
int fat12_list_root(fat12_file_info_t* out, uint32_t max, uint32_t* out_count);

// Read a whole file from the FAT12 root directory.
// Allocates `*out_data` with kmalloc (caller frees with kfree).
// Returns 1 on success.
int fat12_read_file(const char* name, uint8_t** out_data, uint32_t* out_size);

// Create or overwrite a file in the FAT12 root directory.
// Returns 1 on success.
int fat12_write_file(const char* name, const uint8_t* data, uint32_t size);
