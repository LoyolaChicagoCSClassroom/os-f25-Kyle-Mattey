#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <stdint.h>

#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64

void keyboard_init(void);
void handle_keyboard_interrupt(void);
uint8_t keyboard_read_scancode(void);

#endif
