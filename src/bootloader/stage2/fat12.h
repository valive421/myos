#pragma once

#include "stdint.h"
#include "disk.h"
#include "memory.h"

/*
 * FAT12 filesystem metadata used by stage2.
 *
 * Values are parsed from the BPB (boot sector) and then used to locate:
 * - FAT area
 * - root directory area
 * - data area (clusters)
 */

typedef struct
{
    /* Raw BPB fields */
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t fatCount;
    uint16_t rootEntryCount;
    uint16_t totalSectors;
    uint16_t sectorsPerFat;
    uint16_t sectorsPerTrack;
    uint16_t heads;

    /* Derived layout fields */
    uint16_t rootDirSectors;
    uint16_t fatLba;
    uint16_t rootDirLba;
    uint16_t dataLba;
} FAT12_INFO;

/*
 * Reads BPB/FAT/root directory and prepares a FAT12_INFO structure.
 * Returns true on success.
 */
bool FAT12_Initialize(DISK* disk, FAT12_INFO* fs);

/*
 * Loads a file from FAT12 root directory using an 11-byte 8.3 name
 * (example: "KERNEL  BIN") into memory at loadSegment:loadOffset.
 * Returns true on success. Optionally returns file size through fileSizeOut.
 */
bool FAT12_LoadFileByName(DISK* disk,
                          const FAT12_INFO* fs,
                          const char* name11,
                          uint16_t loadSegment,
                          uint16_t loadOffset,
                          uint32_t* fileSizeOut);
