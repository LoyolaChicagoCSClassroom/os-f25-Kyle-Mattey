#include "rprintf.h"
#include "interrupt.h"
#include "keyboard.h"
#include "page.h"
#include "fat.h"
#include "ide.h"
#include <stdint.h>

// HW1: Video memory and putc function
static uint16_t* video_memory = (uint16_t*)0xB8000;
static int cursor_offset = 0;

void putc(int data) {
    char c = (char)data;
    
    if (c == '\n') {
        cursor_offset = ((cursor_offset / 80) + 1) * 80;
    } else if (c == '\r') {
        cursor_offset = (cursor_offset / 80) * 80;
    } else if (c == '\b') {
        if (cursor_offset > 0) {
            cursor_offset--;
            video_memory[cursor_offset] = 0x0F20;
        }
    } else if (c == '\t') {
        cursor_offset = (cursor_offset + 4) & ~(4 - 1);
    } else {
        video_memory[cursor_offset] = (0x0F << 8) | c;
        cursor_offset++;
    }
    
    if (cursor_offset >= 80 * 25) {
        for (int i = 0; i < 80 * 24; i++) {
            video_memory[i] = video_memory[i + 80];
        }
        for (int i = 80 * 24; i < 80 * 25; i++) {
            video_memory[i] = 0x0F20;
        }
        cursor_offset = 80 * 24;
    }
}

void kernel_main(void) {
    // Clear screen
    for (int i = 0; i < 80 * 25; i++) {
        video_memory[i] = 0x0F20;
    }
    cursor_offset = 0;
    
    esp_printf(putc, "CS 310 Operating System\r\n");
    esp_printf(putc, "=======================\r\n\r\n");
    
    // HW1: Terminal Driver
    esp_printf(putc, "[HW1] VGA Terminal initialized\r\n");
    esp_printf(putc, "[HW1] Testing putc() function\r\n");
    esp_printf(putc, "[HW1] Current execution level: Kernel Mode\r\n\r\n");
    
    // HW3: Page Frame Allocator
    esp_printf(putc, "[HW3] Initializing Page Frame Allocator...\r\n");
    init_pfa_list();
    
    struct ppage *pages = allocate_physical_pages(2);
    if (pages) {
        esp_printf(putc, "[HW3] Successfully allocated 2 pages\r\n");
        esp_printf(putc, "[HW3] First page physical address: 0x%x\r\n", 
                   (unsigned int)pages->physical_addr);
        free_physical_pages(pages);
        esp_printf(putc, "[HW3] Pages freed successfully\r\n\r\n");
    }
    
    // HW5: IDE and FAT
    esp_printf(putc, "[HW5] Initializing IDE driver...\r\n");
    ide_init();
    esp_printf(putc, "[HW5] IDE driver initialized\r\n\r\n");
    
    esp_printf(putc, "[HW5] Testing FAT Filesystem Driver...\r\n");
    esp_printf(putc, "\r\n=== FAT Filesystem Test ===\r\n");
    
    if (fatInit() != 0) {
        esp_printf(putc, "ERROR: Failed to initialize FAT\r\n");
    } else {
        esp_printf(putc, "\r\nOpening testfile.txt...\r\n");
        struct file *f = fatOpen("testfile.txt");
        
        if (!f) {
            esp_printf(putc, "ERROR: Could not open testfile.txt\r\n");
        } else {
            esp_printf(putc, "File opened successfully!\r\n");
            esp_printf(putc, "File size: %d bytes\r\n", f->rde.file_size);
            
            char buffer[512];
            int bytes_read = fatRead(f, buffer, sizeof(buffer) - 1);
            
            if (bytes_read < 0) {
                esp_printf(putc, "ERROR: Failed to read file\r\n");
            } else {
                buffer[bytes_read] = '\0';
                esp_printf(putc, "\r\nFile contents (%d bytes):\r\n", bytes_read);
                esp_printf(putc, "---\r\n%s\r\n---\r\n", buffer);
            }
        }
    }
    
    esp_printf(putc, "\r\n[HW2] Setting up interrupts...\r\n");
    idt_init();
    esp_printf(putc, "[HW2] IDT initialized\r\n");
    
    keyboard_init();
    esp_printf(putc, "[HW2] Keyboard initialized\r\n\r\n");
    
    esp_printf(putc, "System initialization complete!\r\n");
    esp_printf(putc, "Press any key to test keyboard...\r\n\r\n");
    
    while (1) {
        __asm__ __volatile__("hlt");
    }
}
