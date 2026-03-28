#include "fat12.h"

/*
 * FAT12 loader for stage2 bootloader.
 *
 * Scope:
 * - Parse BPB from sector 0
 * - Read FAT and root directory into local buffers
 * - Resolve files by 8.3 root entry name
 * - Walk FAT12 cluster chain and read file data to target memory
 */

#define FAT12_SECTOR_SIZE            512
#define FAT12_MAX_FAT_SECTORS        16
#define FAT12_MAX_ROOT_DIR_SECTORS   32

static uint8_t g_bootSector[FAT12_SECTOR_SIZE];
static uint8_t g_fat[FAT12_MAX_FAT_SECTORS * FAT12_SECTOR_SIZE];
static uint8_t g_rootDir[FAT12_MAX_ROOT_DIR_SECTORS * FAT12_SECTOR_SIZE];

static uint16_t read_u16(const uint8_t* p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_u32(const uint8_t* p)
{
    return (uint32_t)read_u16(p) | ((uint32_t)read_u16(p + 2) << 16);
}

static bool str11_eq(const uint8_t* a, const char* b)
{
    uint8_t i;

    for (i = 0; i < 11; i++)
    {
        if (a[i] != (uint8_t)b[i])
            return false;
    }

    return true;
}

static bool FAT12_ReadBPB(DISK* disk, FAT12_INFO* fs)
{
    /* Boot sector is at LBA 0 on FAT12 floppy images. */
    if (!DISK_ReadSectors(disk, 0, 1, g_bootSector))
        return false;

    fs->bytesPerSector = read_u16(&g_bootSector[11]);
    fs->sectorsPerCluster = g_bootSector[13];
    fs->reservedSectors = read_u16(&g_bootSector[14]);
    fs->fatCount = g_bootSector[16];
    fs->rootEntryCount = read_u16(&g_bootSector[17]);
    fs->totalSectors = read_u16(&g_bootSector[19]);
    fs->sectorsPerFat = read_u16(&g_bootSector[22]);
    fs->sectorsPerTrack = read_u16(&g_bootSector[24]);
    fs->heads = read_u16(&g_bootSector[26]);

    if (fs->bytesPerSector != FAT12_SECTOR_SIZE)
        return false;
    if (fs->sectorsPerCluster == 0)
        return false;
    if (fs->sectorsPerFat == 0 || fs->sectorsPerFat > FAT12_MAX_FAT_SECTORS)
        return false;

    fs->rootDirSectors = (uint16_t)((fs->rootEntryCount * 32u + (fs->bytesPerSector - 1u)) / fs->bytesPerSector);
    if (fs->rootDirSectors > FAT12_MAX_ROOT_DIR_SECTORS)
        return false;

    fs->fatLba = fs->reservedSectors;
    fs->rootDirLba = (uint16_t)(fs->fatLba + (uint16_t)(fs->fatCount * fs->sectorsPerFat));
    fs->dataLba = (uint16_t)(fs->rootDirLba + fs->rootDirSectors);

    return true;
}

static bool FAT12_ReadFAT(DISK* disk, const FAT12_INFO* fs)
{
    uint16_t i;

    /* Load first FAT copy only; second copy is ignored for now. */
    for (i = 0; i < fs->sectorsPerFat; i++)
    {
        if (!DISK_ReadSectors(disk, (uint32_t)(fs->fatLba + i), 1, &g_fat[i * FAT12_SECTOR_SIZE]))
            return false;
    }

    return true;
}

static bool FAT12_ReadRootDirectory(DISK* disk, const FAT12_INFO* fs)
{
    uint16_t i;

    for (i = 0; i < fs->rootDirSectors; i++)
    {
        if (!DISK_ReadSectors(disk, (uint32_t)(fs->rootDirLba + i), 1, &g_rootDir[i * FAT12_SECTOR_SIZE]))
            return false;
    }

    return true;
}

static uint16_t FAT12_GetNextCluster(uint16_t cluster)
{
    /* FAT12 uses 12-bit packed entries: offset = n + (n / 2). */
    uint16_t fatOffset = (uint16_t)(cluster + (cluster / 2));
    uint16_t entry = read_u16(&g_fat[fatOffset]);

    if (cluster & 1)
        return (uint16_t)(entry >> 4);

    return (uint16_t)(entry & 0x0FFFu);
}

static bool FAT12_FindRootEntry(const FAT12_INFO* fs,
                                const char* name11,
                                uint16_t* firstClusterOut,
                                uint32_t* fileSizeOut)
{
    uint16_t i;

    for (i = 0; i < fs->rootEntryCount; i++)
    {
        const uint8_t* entry = &g_rootDir[i * 32];
        uint8_t first = entry[0];
        uint8_t attr = entry[11];

        if (first == 0x00)
            break;
        if (first == 0xE5)
            continue;
        if ((attr & 0x0F) == 0x0F)
            continue;
        if (attr & 0x08)
            continue;

        if (str11_eq(entry, name11))
        {
            *firstClusterOut = read_u16(&entry[26]);
            *fileSizeOut = read_u32(&entry[28]);
            return true;
        }
    }

    return false;
}

static bool FAT12_LoadFileToMemory(DISK* disk,
                                   const FAT12_INFO* fs,
                                   uint16_t firstCluster,
                                   uint32_t fileSize,
                                   uint16_t loadSegment,
                                   uint16_t loadOffset)
{
    uint16_t cluster = firstCluster;
    uint16_t sectorCount = 0;
    uint32_t loadedBytes = 0;

    while (cluster >= 2 && cluster < 0x0FF8)
    {
        uint16_t firstSector = (uint16_t)(fs->dataLba + (uint16_t)((cluster - 2) * fs->sectorsPerCluster));
        uint8_t s;

        for (s = 0; s < fs->sectorsPerCluster; s++)
        {
            uint16_t dstOffset = (uint16_t)(loadOffset + sectorCount * fs->bytesPerSector);

            /* Keep all reads inside a single segment window. */
            if (!MEM_CanAccessSegmentRange(dstOffset, fs->bytesPerSector))
                return false;

            if (!DISK_ReadSectorsToSegment(disk,
                                           (uint32_t)(firstSector + s),
                                           1,
                                           loadSegment,
                                           dstOffset))
                return false;

            sectorCount++;
            loadedBytes += fs->bytesPerSector;

            if (loadedBytes >= fileSize)
                return true;
        }

        cluster = FAT12_GetNextCluster(cluster);
    }

    return (fileSize == 0);
}

bool FAT12_Initialize(DISK* disk, FAT12_INFO* fs)
{
    if (!FAT12_ReadBPB(disk, fs))
        return false;

    if (!FAT12_ReadFAT(disk, fs))
        return false;

    if (!FAT12_ReadRootDirectory(disk, fs))
        return false;

    return true;
}

bool FAT12_LoadFileByName(DISK* disk,
                          const FAT12_INFO* fs,
                          const char* name11,
                          uint16_t loadSegment,
                          uint16_t loadOffset,
                          uint32_t* fileSizeOut)
{
    uint16_t firstCluster;
    uint32_t fileSize;

    if (!FAT12_FindRootEntry(fs, name11, &firstCluster, &fileSize))
        return false;

    if (!FAT12_LoadFileToMemory(disk, fs, firstCluster, fileSize, loadSegment, loadOffset))
        return false;

    if (fileSizeOut)
        *fileSizeOut = fileSize;

    return true;
}
