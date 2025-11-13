#ifndef __PAGE_H__
#define __PAGE_H__

// Physical page structure
struct ppage {
    struct ppage *next;
    struct ppage *prev;
    void *physical_addr;
};

// Initialize page frame allocator
void init_pfa_list(void);

// Allocate physical pages
struct ppage *allocate_physical_pages(unsigned int npages);

// Free physical pages
void free_physical_pages(struct ppage *ppage_list);

#endif
