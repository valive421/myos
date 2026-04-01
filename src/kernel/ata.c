#include "ata.h"

#include "io.h"

// Primary ATA I/O ports.
#define ATA_PRIMARY_IO     0x1F0u
#define ATA_PRIMARY_CTRL   0x3F6u

// ATA registers (offsets from ATA_PRIMARY_IO).
#define ATA_REG_DATA       0x00u
#define ATA_REG_ERROR      0x01u
#define ATA_REG_FEATURES   0x01u
#define ATA_REG_SECCOUNT0  0x02u
#define ATA_REG_LBA0       0x03u
#define ATA_REG_LBA1       0x04u
#define ATA_REG_LBA2       0x05u
#define ATA_REG_HDDEVSEL   0x06u
#define ATA_REG_COMMAND    0x07u
#define ATA_REG_STATUS     0x07u

// Commands.
#define ATA_CMD_IDENTIFY        0xECu
#define ATA_CMD_READ_SECTORS    0x20u
#define ATA_CMD_WRITE_SECTORS   0x30u
#define ATA_CMD_CACHE_FLUSH     0xE7u

// Status bits.
#define ATA_SR_BSY  0x80u
#define ATA_SR_DRDY 0x40u
#define ATA_SR_DF   0x20u
#define ATA_SR_DRQ  0x08u
#define ATA_SR_ERR  0x01u

static int g_AtaPresent = 0;

static uint8_t ata_status(void)
{
    return inb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_STATUS));
}

static uint8_t ata_altstatus(void)
{
    return inb((uint16_t)ATA_PRIMARY_CTRL);
}

static void ata_delay_400ns(void)
{
    // 400ns delay by reading the alternate status port 4 times.
    (void)ata_altstatus();
    (void)ata_altstatus();
    (void)ata_altstatus();
    (void)ata_altstatus();
}

static int ata_wait_not_bsy(uint32_t spin)
{
    while (spin--)
    {
        uint8_t st = ata_status();
        if ((st & ATA_SR_BSY) == 0)
            return 1;
    }
    return 0;
}

static int ata_wait_drq(uint32_t spin)
{
    while (spin--)
    {
        uint8_t st = ata_status();
        if (st & ATA_SR_ERR)
            return 0;
        if (st & ATA_SR_DF)
            return 0;
        if (((st & ATA_SR_BSY) == 0) && (st & ATA_SR_DRQ))
            return 1;
    }
    return 0;
}

static void ata_select_master(uint8_t high4)
{
    // 0xE0 = master + LBA mode.
    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL), (uint8_t)(0xE0u | (high4 & 0x0Fu)));
    ata_delay_400ns();
}

int ata_present(void)
{
    return g_AtaPresent;
}

int ata_init(void)
{
    // IDENTIFY works only if a device is present.
    // If status reads as 0x00 or 0xFF, treat as no device.
    ata_select_master(0);

    uint8_t st = ata_status();
    if (st == 0x00u || st == 0xFFu)
    {
        g_AtaPresent = 0;
        return 0;
    }

    // Zero out LBA and sector count as recommended for IDENTIFY.
    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0), 0);
    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_LBA0), 0);
    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_LBA1), 0);
    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_LBA2), 0);

    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_COMMAND), ATA_CMD_IDENTIFY);
    ata_delay_400ns();

    st = ata_status();
    if (st == 0x00u || st == 0xFFu)
    {
        g_AtaPresent = 0;
        return 0;
    }

    // Wait for DRQ.
    if (!ata_wait_drq(1000000u))
    {
        g_AtaPresent = 0;
        return 0;
    }

    // Read and discard identify data.
    uint16_t identify[256];
    insw((uint16_t)(ATA_PRIMARY_IO + ATA_REG_DATA), identify, 256u);

    g_AtaPresent = 1;
    return 1;
}

int ata_read_sectors(uint32_t lba, uint8_t count, void* out)
{
    if (!out)
        return 0;

    if ((lba >> 28) != 0)
        return 0;

    if (count == 0)
        return 0;

    if (!g_AtaPresent && !ata_init())
        return 0;

    if (!ata_wait_not_bsy(1000000u))
        return 0;

    ata_select_master((uint8_t)((lba >> 24) & 0x0Fu));

    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0), count);
    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_LBA0), (uint8_t)(lba & 0xFFu));
    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_LBA1), (uint8_t)((lba >> 8) & 0xFFu));
    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_LBA2), (uint8_t)((lba >> 16) & 0xFFu));

    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_COMMAND), ATA_CMD_READ_SECTORS);

    uint16_t* dst = (uint16_t*)out;
    for (uint32_t s = 0; s < count; s++)
    {
        if (!ata_wait_drq(1000000u))
            return 0;

        insw((uint16_t)(ATA_PRIMARY_IO + ATA_REG_DATA), &dst[s * 256u], 256u);
        ata_delay_400ns();
    }

    return 1;
}

int ata_write_sectors(uint32_t lba, uint8_t count, const void* in)
{
    if (!in)
        return 0;

    if ((lba >> 28) != 0)
        return 0;

    if (count == 0)
        return 0;

    if (!g_AtaPresent && !ata_init())
        return 0;

    if (!ata_wait_not_bsy(1000000u))
        return 0;

    ata_select_master((uint8_t)((lba >> 24) & 0x0Fu));

    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0), count);
    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_LBA0), (uint8_t)(lba & 0xFFu));
    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_LBA1), (uint8_t)((lba >> 8) & 0xFFu));
    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_LBA2), (uint8_t)((lba >> 16) & 0xFFu));

    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_COMMAND), ATA_CMD_WRITE_SECTORS);

    const uint16_t* src = (const uint16_t*)in;
    for (uint32_t s = 0; s < count; s++)
    {
        if (!ata_wait_drq(1000000u))
            return 0;

        outsw((uint16_t)(ATA_PRIMARY_IO + ATA_REG_DATA), &src[s * 256u], 256u);
        ata_delay_400ns();

        if (!ata_wait_not_bsy(1000000u))
            return 0;

        uint8_t st = ata_status();
        if (st & (ATA_SR_ERR | ATA_SR_DF))
            return 0;
    }

    // Ask drive to flush its cache (QEMU should still persist without it, but this is cheap).
    outb((uint16_t)(ATA_PRIMARY_IO + ATA_REG_COMMAND), ATA_CMD_CACHE_FLUSH);
    ata_delay_400ns();
    (void)ata_wait_not_bsy(2000000u);

    return 1;
}
