#include "page.h"
#include <stdint.h>

#define NUM_PAGES 128
#define PAGE_SIZE (2 * 1024 * 1024)
#define PHYS_MEM_START 0x00100000

struct ppage physical_page_array[NUM_PAGES];
static struct ppage *free_physical_pages = 0;

void init_pfa_list(void) {
    for (int i = 0; i < NUM_PAGES; i++) {
        physical_page_array[i].physical_addr = (void*)(PHYS_MEM_START + (i * PAGE_SIZE));
        physical_page_array[i].next = (i < NUM_PAGES - 1) ? &physical_page_array[i + 1] : 0;
        physical_page_array[i].prev = (i > 0) ? &physical_page_array[i - 1] : 0;
    }
    free_physical_pages = &physical_page_array[0];
}

struct ppage *allocate_physical_pages(unsigned int npages) {
    if (npages == 0 || free_physical_pages == 0) {
        return 0;
    }
    
    struct ppage *current = free_physical_pages;
    unsigned int count = 0;
    while (current != 0 && count < npages) {
        current = current->next;
        count++;
    }
    
    if (count < npages) {
        return 0;
    }
    
    struct ppage *allocated_list = free_physical_pages;
    struct ppage *last_allocated = free_physical_pages;
    
    for (unsigned int i = 0; i < npages - 1; i++) {
        last_allocated = last_allocated->next;
    }
    
    free_physical_pages = last_allocated->next;
    if (free_physical_pages != 0) {
        free_physical_pages->prev = 0;
    }
    
    last_allocated->next = 0;
    
    return allocated_list;
}

void free_physical_pages(struct ppage *ppage_list) {
    if (ppage_list == 0) {
        return;
    }
    
    struct ppage *last = ppage_list;
    while (last->next != 0) {
        last = last->next;
    }
    
    last->next = free_physical_pages;
    if (free_physical_pages != 0) {
        free_physical_pages->prev = last;
    }
    free_physical_pages = ppage_list;
    ppage_list->prev = 0;
}
