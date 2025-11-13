#include "fat.h"
#include "ide.h"
#include "rprintf.h"
#include <stdint.h>

extern void putc(int data);

static char boot_sector_buf[SECTOR_SIZE];
static struct boot_sector *bs;
static uint8_t fat_table[8 * SECTOR_SIZE];
static uint32_t root_sector;
static uint32_t data_start_sector;

static int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static char toupper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }
    return c;
}

static void memcpy_custom(void *dest, const void *src, uint32_t n) {
    char *d = (char*)dest;
    const char *s = (const char*)src;
    while (n--) {
        *d++ = *s++;
    }
}

static void memset_custom(void *s, int c, uint32_t n) {
    unsigned char *p = (unsigned char*)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
}

int fatInit(void) {
    if (ata_lba_read(0, (uint8_t*)boot_sector_buf, 1) != 0) {
        return -1;
    }
    
    bs = (struct boot_sector*)boot_sector_buf;
    
    if (bs->boot_signature != 0xAA55) {
        esp_printf(putc, "Invalid boot signature: 0x%x\r\n", bs->boot_signature);
        return -1;
    }
    
    if (strcmp(bs->fs_type, "FAT12   ") != 0 && strcmp(bs->fs_type, "FAT16   ") != 0) {
        esp_printf(putc, "Invalid FS type\r\n");
        return -1;
    }
    
    uint32_t fat_start = bs->num_reserved_sectors + bs->num_hidden_sectors;
    uint32_t fat_sectors = bs->num_sectors_per_fat;
    
    if (fat_sectors > 8) {
        fat_sectors = 8;
    }
    
    if (ata_lba_read(fat_start, fat_table, fat_sectors) != 0) {
        return -1;
    }
    
    root_sector = bs->num_reserved_sectors + 
                  (bs->num_fat_tables * bs->num_sectors_per_fat) + 
                  bs->num_hidden_sectors;
    
    uint32_t root_dir_sectors = (bs->num_root_dir_entries * 32 + bs->bytes_per_sector - 1) / bs->bytes_per_sector;
    data_start_sector = root_sector + root_dir_sectors;
    
    esp_printf(putc, "FAT initialized successfully\r\n");
    esp_printf(putc, "Root sector: %d\r\n", root_sector);
    esp_printf(putc, "Data start: %d\r\n", data_start_sector);
    
    return 0;
}

uint16_t fat_get_next_cluster(uint16_t cluster) {
    uint32_t fat_offset = cluster + (cluster / 2);
    uint16_t next_cluster = *((uint16_t*)&fat_table[fat_offset]);
    
    if (cluster & 1) {
        next_cluster >>= 4;
    } else {
        next_cluster &= 0x0FFF;
    }
    
    return next_cluster;
}

uint32_t cluster_to_lba(uint16_t cluster) {
    return data_start_sector + ((cluster - 2) * bs->num_sectors_per_cluster);
}

struct file* fatOpen(const char *filename) {
    static struct file open_file;
    char rde_buffer[SECTOR_SIZE];
    
    char name_83[11];
    memset_custom(name_83, ' ', 11);
    
    int i = 0, j = 0;
    while (filename[i] && filename[i] != '.' && j < 8) {
        name_83[j++] = toupper(filename[i++]);
    }
    
    if (filename[i] == '.') {
        i++;
        j = 8;
        while (filename[i] && j < 11) {
            name_83[j++] = toupper(filename[i++]);
        }
    }
    
    uint32_t entries_per_sector = SECTOR_SIZE / 32;
    uint32_t total_sectors = (bs->num_root_dir_entries * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE;
    
    for (uint32_t sector = 0; sector < total_sectors; sector++) {
        if (ata_lba_read(root_sector + sector, (uint8_t*)rde_buffer, 1) != 0) {
            return 0;
        }
        
        struct root_directory_entry *entries = (struct root_directory_entry*)rde_buffer;
        
        for (uint32_t i = 0; i < entries_per_sector; i++) {
            if (entries[i].file_name[0] == 0x00) {
                return 0;
            }
            
            if (entries[i].file_name[0] == 0xE5) {
                continue;
            }
            
            int match = 1;
            for (int k = 0; k < 11; k++) {
                if (entries[i].file_name[k] != name_83[k]) {
                    match = 0;
                    break;
                }
            }
            
            if (match) {
                memcpy_custom(&open_file.rde, &entries[i], sizeof(struct root_directory_entry));
                open_file.start_cluster = entries[i].cluster;
                open_file.position = 0;
                return &open_file;
            }
        }
    }
    
    return 0;
}

int fatRead(struct file *f, void *buffer, uint32_t size) {
    if (!f) {
        return -1;
    }
    
    uint32_t bytes_read = 0;
    uint32_t bytes_to_read = size;
    
    if (bytes_to_read > f->rde.file_size - f->position) {
        bytes_to_read = f->rde.file_size - f->position;
    }
    
    uint16_t cluster = f->start_cluster;
    uint32_t cluster_size = bs->num_sectors_per_cluster * SECTOR_SIZE;
    char cluster_buffer[4096];
    
    uint32_t skip_clusters = f->position / cluster_size;
    for (uint32_t i = 0; i < skip_clusters; i++) {
        cluster = fat_get_next_cluster(cluster);
        if (cluster >= 0xFF8) {
            return bytes_read;
        }
    }
    
    uint32_t offset_in_cluster = f->position % cluster_size;
    
    while (bytes_read < bytes_to_read && cluster < 0xFF8) {
        uint32_t lba = cluster_to_lba(cluster);
        
        if (ata_lba_read(lba, (uint8_t*)cluster_buffer, bs->num_sectors_per_cluster) != 0) {
            return bytes_read;
        }
        
        uint32_t bytes_in_cluster = cluster_size - offset_in_cluster;
        if (bytes_in_cluster > bytes_to_read - bytes_read) {
            bytes_in_cluster = bytes_to_read - bytes_read;
        }
        
        memcpy_custom((char*)buffer + bytes_read, cluster_buffer + offset_in_cluster, bytes_in_cluster);
        bytes_read += bytes_in_cluster;
        f->position += bytes_in_cluster;
        
        offset_in_cluster = 0;
        cluster = fat_get_next_cluster(cluster);
    }
    
    return bytes_read;
}
