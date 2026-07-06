#include "fs/ata.h"
#include "arch/io.h"

#define ATA_PRIMARY_IO      0x1F0u
#define ATA_PRIMARY_CTRL    0x3F6u
#define ATA_REG_DATA        0u
#define ATA_REG_ERROR       1u
#define ATA_REG_SECCOUNT0   2u
#define ATA_REG_LBA0        3u
#define ATA_REG_LBA1        4u
#define ATA_REG_LBA2        5u
#define ATA_REG_HDDEVSEL    6u
#define ATA_REG_COMMAND     7u
#define ATA_REG_STATUS      7u

#define ATA_CMD_READ_PIO    0x20u
#define ATA_CMD_WRITE_PIO   0x30u
#define ATA_CMD_CACHE_FLUSH 0xE7u
#define ATA_CMD_IDENTIFY    0xECu

#define ATA_SR_ERR          0x01u
#define ATA_SR_DRQ          0x08u
#define ATA_SR_BSY          0x80u

#define ATA_POLL_LIMIT      100000u

static uint8_t ata_status(void)
{
    return inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
}

static void ata_400ns_delay(void)
{
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
}

static int ata_wait_ready(void)
{
    for (uint32_t i = 0; i < ATA_POLL_LIMIT; ++i) {
        uint8_t status = ata_status();
        if ((status & ATA_SR_BSY) == 0) {
            return (status & ATA_SR_ERR) ? -1 : 0;
        }
    }
    return -1;
}

static int ata_wait_drq(void)
{
    for (uint32_t i = 0; i < ATA_POLL_LIMIT; ++i) {
        uint8_t status = ata_status();
        if ((status & ATA_SR_ERR) != 0) {
            return -1;
        }
        if ((status & ATA_SR_BSY) == 0 && (status & ATA_SR_DRQ) != 0) {
            return 0;
        }
    }
    return -1;
}

int ata_identify_primary_master(void)
{
    outb(ATA_PRIMARY_CTRL, 0x02);
    outb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xA0);
    ata_400ns_delay();
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA0, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA1, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA2, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_400ns_delay();

    if (ata_status() == 0) {
        return -1;
    }

    if (ata_wait_drq() != 0) {
        return -1;
    }

    for (uint32_t i = 0; i < ATA_SECTOR_SIZE / 2u; ++i) {
        (void)inw(ATA_PRIMARY_IO + ATA_REG_DATA);
    }

    return 0;
}

static void ata_select_lba(uint32_t lba, uint8_t sectors)
{
    outb(ATA_PRIMARY_CTRL, 0x02);
    outb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL,
         (uint8_t)(0xE0u | ((lba >> 24) & 0x0Fu)));
    ata_400ns_delay();
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, sectors);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA0, (uint8_t)(lba & 0xFFu));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFFu));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFFu));
}

int ata_read_sectors(uint32_t lba, uint8_t sectors, void *buffer)
{
    if (sectors == 0 || buffer == 0 || (lba >> 28) != 0) {
        return -1;
    }

    uint16_t *out = (uint16_t *)buffer;
    if (ata_wait_ready() != 0) {
        return -1;
    }

    ata_select_lba(lba, sectors);
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    for (uint32_t s = 0; s < sectors; ++s) {
        if (ata_wait_drq() != 0) {
            return -1;
        }

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE / 2u; ++i) {
            *out++ = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
        }
        ata_400ns_delay();
    }

    return 0;
}

int ata_write_sectors(uint32_t lba, uint8_t sectors, const void *buffer)
{
    if (sectors == 0 || buffer == 0 || (lba >> 28) != 0) {
        return -1;
    }

    const uint16_t *in = (const uint16_t *)buffer;
    if (ata_wait_ready() != 0) {
        return -1;
    }

    ata_select_lba(lba, sectors);
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    for (uint32_t s = 0; s < sectors; ++s) {
        if (ata_wait_drq() != 0) {
            return -1;
        }

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE / 2u; ++i) {
            outw(ATA_PRIMARY_IO + ATA_REG_DATA, *in++);
        }
        ata_400ns_delay();
    }

    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    return ata_wait_ready();
}
