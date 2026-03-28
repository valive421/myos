#include "disk.h"
#include "x86.h"

static uint16_t DISK_GetDataSegment(void)
{
    uint16_t segment;

    __asm__ __volatile__("mov %%ds, %0" : "=rm"(segment));
    return segment;
}


// Initializes a DISK structure with the parameters of the specified drive number
bool DISK_Initialize(DISK* disk, uint8_t driveNumber)
{
    uint8_t driveType;
    uint16_t cylinders, sectors, heads;

    if (!x86_Disk_GetDriveParams(driveNumber, &driveType, &cylinders, &sectors, &heads))
        return false;

    disk->id = driveNumber;
    disk->cylinders = cylinders + 1;
    disk->heads = heads + 1;
    disk->sectors = sectors;

    return true;
}



// Converts a Logical Block Address (LBA) to Cylinder-Head-Sector (CHS) format
void DISK_LBA2CHS(DISK* disk, uint32_t lba, uint16_t* cylinderOut, uint16_t* sectorOut, uint16_t* headOut)
{
    uint64_t lba_div_sectors;
    uint32_t lba_mod_sectors;
    uint64_t cylinder;
    uint32_t head;

    x86_div64_32((uint64_t)lba, disk->sectors, &lba_div_sectors, &lba_mod_sectors);
    x86_div64_32(lba_div_sectors, disk->heads, &cylinder, &head);

    // sector = (LBA % sectors per track + 1)
    *sectorOut = (uint16_t)lba_mod_sectors + 1;

    // cylinder = (LBA / sectors per track) / heads
    *cylinderOut = (uint16_t)cylinder;

    // head = (LBA / sectors per track) % heads
    *headOut = (uint16_t)head;
}


// Reads sectors from the disk using CHS addressing, with retries on failure
bool DISK_ReadSectorsToSegment(DISK* disk, uint32_t lba, uint8_t sectors, uint16_t segment, uint16_t offset)
{
    uint16_t cylinder, sector, head;

    DISK_LBA2CHS(disk, lba, &cylinder, &sector, &head);

    for (int i = 0; i < 3; i++)
    {
        if (x86_Disk_Read(disk->id, cylinder, sector, head, sectors, segment, offset))
            return true;

        x86_Disk_Reset(disk->id);
    }

    return false;
}

// Reads sectors into a near pointer in the current data segment
bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t sectors, void* dataOut)
{
    return DISK_ReadSectorsToSegment(disk,
                                     lba,
                                     sectors,
                                     DISK_GetDataSegment(),
                                     (uint16_t)(uintptr_t)dataOut);
}
