#ifndef __IO_H__
#define __IO_H__

#include <stdint.h>

// I/O port functions
static inline uint8_t inb(uint16_t port) {
    uint8_t rv;
    __asm__ __volatile__("inb %1, %0" : "=a" (rv) : "dN" (port));
    return rv;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a" (val), "dN" (port));
}

#endif
