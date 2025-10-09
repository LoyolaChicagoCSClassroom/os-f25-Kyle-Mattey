

#include <stdint.h>
#include "rprintf.h"

/* ---------- Multiboot header (leave as provided) ---------- */
#define MULTIBOOT2_HEADER_MAGIC 0xe85250d6
const unsigned int multiboot_header[] __attribute__((section(".multiboot"))) =
    { MULTIBOOT2_HEADER_MAGIC, 0, 16, -(16 + MULTIBOOT2_HEADER_MAGIC), 0, 12 };

/* ---------- VGA text buffer ---------- */
#define VGA_ADDRESS 0xB8000
#define VGA_COLS 80
#define VGA_ROWS 25

struct termbuf { uint8_t ascii; uint8_t color; };
static volatile struct termbuf *const vram = (volatile struct termbuf*)VGA_ADDRESS;
static int x = 0, y = 0;

static void scroll(void) {
    for (int row = 1; row < VGA_ROWS; row++)
        for (int col = 0; col < VGA_COLS; col++)
            vram[(row - 1) * VGA_COLS + col] = vram[row * VGA_COLS + col];
    for (int col = 0; col < VGA_COLS; col++) {
        vram[(VGA_ROWS - 1) * VGA_COLS + col].ascii = ' ';
        vram[(VGA_ROWS - 1) * VGA_COLS + col].color = 7;
    }
}

int putc(int c) {
    if (c == '\n') {
        x = 0; y++;
    } else {
        vram[y * VGA_COLS + x].ascii = (uint8_t)c;
        vram[y * VGA_COLS + x].color = 7;
        x++;
        if (x >= VGA_COLS) { x = 0; y++; }
    }
    if (y >= VGA_ROWS) { scroll(); y = VGA_ROWS - 1; }
    return 0;
}

/* ---------- Port I/O ---------- */
static inline uint8_t inb(uint16_t port) {
    uint8_t val; __asm__ volatile("inb %1,%0" : "=a"(val) : "dN"(port));
    return val;
}
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0,%1" :: "a"(val), "dN"(port));
}

/* ---------- PS/2 controller ---------- */
#define PS2_DATA    0x60
#define PS2_STATUS  0x64
#define STATUS_OBF  0x01  /* Output Buffer Full */

/* Basic scancode -> ASCII map for make codes (no shift) */
static const char keymap[128] = {
  0,27,'1','2','3','4','5','6','7','8','9','0','-','=', '\b',
  '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
  'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
  'z','x','c','v','b','n','m',',','.','/',0,0,0,' ',
};

/* Hex print helper */
static void puthex8(uint8_t v) {
    static const char H[] = "0123456789ABCDEF";
    putc(H[(v >> 4) & 0xF]);
    putc(H[v & 0xF]);
}

/* ---------- Kernel entry ---------- */
void kernel_main(void) {
    esp_printf(putc, "COMP310 HW2: Keyboard Polling\n");
    esp_printf(putc, "Type keys. ASCII prints when mapped; otherwise SC:0xHH\n");

    int e0 = 0; /* track E0 extended prefix */
    for (;;) {
        if ((inb(PS2_STATUS) & STATUS_OBF) == 0) {
            __asm__ volatile("pause");
            continue;
        }

        uint8_t sc = inb(PS2_DATA);

        /* Handle prefixes and break codes */
        if (sc == 0xE0) { e0 = 1; continue; }      /* extended prefix */
        if (sc & 0x80)  { e0 = 0; continue; }      /* ignore key releases */

        if (e0) {
            esp_printf(putc, "[E0] SC:0x"); puthex8(sc); putc('\n');
            e0 = 0;
            continue;
        }

        char ch = (sc < 128) ? keymap[sc] : 0;
        if (ch) {
            if (ch == '\b') {
                if (x > 0) { x--; vram[y * VGA_COLS + x].ascii = ' '; }
            } else {
                putc(ch);
            }
        } else {
            esp_printf(putc, "SC:0x"); puthex8(sc); putc('\n');
        }
    }
}
