#ifndef FS_ATA_H
#define FS_ATA_H

#include <stdint.h>

#define ATA_SECTOR_SIZE 512u

int ata_identify_primary_master(void);
int ata_read_sectors(uint32_t lba, uint8_t sectors, void *buffer);
int ata_write_sectors(uint32_t lba, uint8_t sectors, const void *buffer);

#endif
