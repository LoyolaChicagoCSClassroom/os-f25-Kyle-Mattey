#ifndef __IDE_H__
#define __IDE_H__

#include <stdint.h>

#define SECTOR_SIZE 512

#define IDE_DATA        0x1F0
#define IDE_ERROR       0x1F1
#define IDE_SECTOR_CNT  0x1F2
#define IDE_LBA_LO      0x1F3
#define IDE_LBA_MID     0x1F4
#define IDE_LBA_HI      0x1F5
#define IDE_DRIVE       0x1F6
#define IDE_COMMAND     0x1F7
#define IDE_STATUS      0x1F7

#define IDE_CMD_READ_SECTORS  0x20
#define IDE_CMD_WRITE_SECTORS 0x30

#define IDE_STATUS_BSY  0x80
#define IDE_STATUS_DRDY 0x40
#define IDE_STATUS_DRQ  0x08
#define IDE_STATUS_ERR  0x01

void ide_init(void);
int ata_lba_read(uint32_t lba, uint8_t *buffer, uint32_t nsectors);
void ide_wait(void);

extern void ide_read_buffer(uint16_t *buffer, uint32_t count);

#endif
