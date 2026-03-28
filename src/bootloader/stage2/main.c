#include "stdint.h"
#include "disk.h"
#include "fat12.h"
#include "memory.h"
#include "stdio.h"

#define KERNEL_LOAD_SEGMENT          0x4000
#define KERNEL_LOAD_OFFSET           0x0000

void _cdecl cstart_(uint16_t bootDrive)
{
    DISK disk;
    FAT12_INFO fs;
    uint32_t kernelFileSize;
    void (_cdecl far* kernelEntry)(void);

    puts("Stage2: FAT12 loader\r\n");

    if (!DISK_Initialize(&disk, (uint8_t)bootDrive))
    {
        puts("Disk init failed\r\n");
        for (;;);
    }

    if (!FAT12_Initialize(&disk, &fs))
    {
        puts("FAT12 init failed\r\n");
        for (;;);
    }

    if (!FAT12_LoadFileByName(&disk, &fs, "KERNEL  BIN", KERNEL_LOAD_SEGMENT, KERNEL_LOAD_OFFSET, &kernelFileSize))
    {
        puts("Kernel load failed\r\n");
        for (;;);
    }

    puts("Kernel loaded\r\n");

    kernelEntry = (void (_cdecl far*)(void))MEM_FarPtr(KERNEL_LOAD_SEGMENT, KERNEL_LOAD_OFFSET);
    kernelEntry();

    for (;;);
}
