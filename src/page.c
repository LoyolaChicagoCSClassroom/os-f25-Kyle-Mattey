cat > src/page.c <<'EOF'
#include "page.h"
#include <stddef.h>
#include <stdint.h>

/* Statically allocate 128 physical page descriptors (2MB each = 256MB total) */
static struct ppage physical_page_array[128];
static struct ppage *free_physical_pages = NULL;

/* Initialize the linked list of free physical pages */
void init_pfa_list(void) {
    for (int i = 0; i < 128; i++) {
        physical_page_array[i].physical_addr = (void *)((uintptr_t)i * 0x200000);
        physical_page_array[i].next = (i < 127) ? &physical_page_array[i + 1] : NULL;
        physical_page_array[i].prev = (i > 0) ? &physical_page_array[i - 1] : NULL;
    }
    free_physical_pages = &physical_page_array[0];
}

/* Helper: unlink a single page from the head of the free list */
static struct ppage *unlink_head(void) {
    if (free_physical_pages == NULL) return NULL;
    struct ppage *p = free_physical_pages;
    free_physical_pages = p->next;
    if (free_physical_pages) free_physical_pages->prev = NULL;
    p->next = NULL;
    p->prev = NULL;
    return p;
}

/* Allocate npages contiguous physical pages */
struct ppage *allocate_physical_pages(unsigned int npages) {
    if (npages == 0) return NULL;

    struct ppage *head = NULL;
    struct ppage *tail = NULL;

    for (unsigned int i = 0; i < npages; i++) {
        struct ppage *p = unlink_head();
        if (!p) {
            /* Out of free pages — rollback */
            if (head) {
                free_physical_pages(head);
            }
            return NULL;
        }
        if (!head) {
            head = p;
            tail = p;
        } else {
            tail->next = p;
            p->prev = tail;
            tail = p;
        }
    }
    return head;
}

/* Return a list of pages back to the free list */
void free_physical_pages(struct ppage *ppage_list) {
    if (!ppage_list) return;

    struct ppage *tail = ppage_list;
    while (tail->next) tail = tail->next;

    /* Attach to front of free list */
    tail->next = free_physical_pages;
    if (free_physical_pages)
        free_physical_pages->prev = tail;

    ppage_list->prev = NULL;
    free_physical_pages = ppage_list;
}
EOF
