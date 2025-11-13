#include "keyboard.h"
#include "io.h"
#include "rprintf.h"

extern void putc(int data);

static unsigned char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void keyboard_init(void) {
    uint8_t mask = inb(0x21);
    mask &= ~0x02;
    outb(0x21, mask);
}

uint8_t keyboard_read_scancode(void) {
    while ((inb(PS2_STATUS_PORT) & 0x01) == 0);
    return inb(PS2_DATA_PORT);
}

void handle_keyboard_interrupt(void) {
    uint8_t scancode = inb(PS2_DATA_PORT);
    
    if (scancode < 0x80) {
        char ascii = scancode_to_ascii[scancode];
        if (ascii != 0) {
            putc(ascii);
        }
    }
}
