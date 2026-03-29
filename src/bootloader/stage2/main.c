#include "stdint.h"
#include "disk.h"
#include "fat12.h"
#include "memory.h"
#include "stdio.h"
#include "x86.h"

#define KERNEL_LOAD_SEGMENT          0x4000
#define KERNEL_LOAD_OFFSET           0x0000

void cstart_(uint16_t bootDrive)
{
    DISK disk;
    FAT12_INFO fs;
    uint32_t kernelFileSize;

    x86_Serial_Init();

    printf("Stage2: FAT12 loader\r\n");
    printf("[S2] disk init...\r\n");

    if (!DISK_Initialize(&disk, (uint8_t)bootDrive))
    {
        printf("Disk init failed\r\n");
        for (;;);
    }

    printf("[S2] FAT12 init...\r\n");

    if (!FAT12_Initialize(&disk, &fs))
    {
        printf("FAT12 init failed\r\n");
        for (;;);
    }

    printf("[S2] load KERNEL. BIN...\r\n");

    if (!FAT12_LoadFileByName(&disk, &fs, "KERNEL  BIN", KERNEL_LOAD_SEGMENT, KERNEL_LOAD_OFFSET, &kernelFileSize))
    {
        printf("Kernel load failed\r\n");
        for (;;);
    }

    printf("Kernel loaded\r\n");

    x86_JumpToKernel(KERNEL_LOAD_SEGMENT, KERNEL_LOAD_OFFSET);

    for (;;);
}
