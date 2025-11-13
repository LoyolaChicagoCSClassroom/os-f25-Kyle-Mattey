#include "interrupt.h"
#include "keyboard.h"

#define IDT_ENTRIES 256

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

extern void idt_load(uint32_t);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_lo = base & 0xFFFF;
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint32_t)&idt;
    
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    idt_set_gate(0x21, (uint32_t)isr_keyboard, 0x08, 0x8E);
    idt_set_gate(0x20, (uint32_t)isr_timer, 0x08, 0x8E);
    
    idt_load((uint32_t)&idtp);
    
    __asm__ __volatile__("sti");
}

void keyboard_handler(void) {
    handle_keyboard_interrupt();
}

void timer_handler(void) {
    // Timer handler implementation
}
