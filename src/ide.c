#include "ide.h"
#include "io.h"

void ide_init(void) {
    outb(IDE_DRIVE, 0xA0);
}

void ide_wait(void) {
    uint8_t status;
    while ((status = inb(IDE_STATUS)) & IDE_STATUS_BSY);
    while (!((status = inb(IDE_STATUS)) & IDE_STATUS_DRDY));
}

int ata_lba_read(uint32_t lba, uint8_t *buffer, uint32_t nsectors) {
    if (nsectors == 0 || nsectors > 256) {
        return -1;
    }
    
    ide_wait();
    
    outb(IDE_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(IDE_SECTOR_CNT, nsectors == 256 ? 0 : nsectors);
    outb(IDE_LBA_LO, lba & 0xFF);
    outb(IDE_LBA_MID, (lba >> 8) & 0xFF);
    outb(IDE_LBA_HI, (lba >> 16) & 0xFF);
    outb(IDE_COMMAND, IDE_CMD_READ_SECTORS);
    
    for (uint32_t i = 0; i < nsectors; i++) {
        ide_wait();
        
        uint8_t status = inb(IDE_STATUS);
        if (status & IDE_STATUS_ERR) {
            return -1;
        }
        
        while (!(inb(IDE_STATUS) & IDE_STATUS_DRQ));
        
        ide_read_buffer((uint16_t*)(buffer + i * SECTOR_SIZE), SECTOR_SIZE / 2);
    }
    
    return 0;
}
