cat > src/page.h <<'EOF'
#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

struct ppage {
    struct ppage *next;
    struct ppage *prev;
    void *physical_addr;
};

/* Initialize the page frame allocator list */
void init_pfa_list(void);

/* Allocate npages contiguous physical pages from the free list */
struct ppage *allocate_physical_pages(unsigned int npages);

/* Return a list of pages back to the free list */
void free_physical_pages(struct ppage *ppage_list);

#endif // PAGE_H
EOF
