#include <stdint.h>
#include <stdbool.h>
#include <arch/x86_64/io.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/pic.h>
#include <drivers/keyboard.h>

#define KBD_DATA 0x60   // the keyboard controller's data port

// Scancodes for the keys we care about.
#define SC_LSHIFT   0x2A
#define SC_RSHIFT   0x36
#define SC_ALT      0x38    // left Alt; with the 0xE0 prefix it's AltGr
#define SC_RELEASE  0x80    // bit 7 set = key released
#define SC_EXTENDED 0xE0    // prefix for arrows, right ctrl, AltGr, etc.

#define KEYMAP_SIZE 0x57    // big enough to reach the < > key at 0x56

// Scancode -> character, French AZERTY. 0 = no character.
// The position in the array is the scancode: index 0x10 is the 'a' key.
static const char normal_map[KEYMAP_SIZE] = {
    0,    27,  '&', 0,   '"', '\'', '(', '-', 0,   '_', 0,   0,   ')', '=', '\b',
    '\t', 'a', 'z', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '^', '$', '\n',
    0,    'q', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 0,   0,
    0,    '*', 'w', 'x', 'c', 'v', 'b', 'n', ',', ';', ':', '!', 0,
    '*',  0,   ' ',
    [0x56] = '<',
};

static const char shift_map[KEYMAP_SIZE] = {
    0,    27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 0,   '+', '\b',
    '\t', 'A', 'Z', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 0,   0,   '\n',
    0,    'Q', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'M', '%', 0,
    0,    0,   'W', 'X', 'C', 'V', 'B', 'N', '?', '.', '/', 0,   0,
    '*',  0,   ' ',
    [0x56] = '>',
};

// AltGr + key. Only a few keys have something here.
static const char altgr_map[KEYMAP_SIZE] = {
    [0x03] = '~', [0x04] = '#', [0x05] = '{', [0x06] = '[', [0x07] = '|',
    [0x08] = '`', [0x09] = '\\', [0x0A] = '^', [0x0B] = '@', [0x0C] = ']',
    [0x0D] = '}',
};

// A small circular buffer between the interrupt (which adds characters)
// and the rest of the kernel (which takes them out).
#define BUFFER_SIZE 128
static volatile char     buffer[BUFFER_SIZE];
static volatile uint32_t head;   // where the interrupt writes next
static volatile uint32_t tail;   // where keyboard_getchar reads next

static bool shift_held;
static bool altgr_held;
static bool extended;

static void buffer_push(char c) {
    uint32_t next = (head + 1) % BUFFER_SIZE;
    if (next != tail) {         // if the buffer is full, drop the key
        buffer[head] = c;
        head = next;
    }
}

// Runs on every IRQ 1: one byte per key press or release.
static void keyboard_irq(interrupt_frame_t *frame) {
    (void)frame;

    // Always read the byte, or the keyboard stops sending more.
    uint8_t sc = inb(KBD_DATA);

    if (sc == SC_EXTENDED) {
        extended = true;
        return;
    }

    bool released = sc & SC_RELEASE;
    uint8_t key   = sc & ~SC_RELEASE;

    // A key after 0xE0. AltGr is "0xE0 + Alt"; the rest (arrows...) we still skip.
    if (extended) {
        extended = false;
        if (key == SC_ALT) {
            altgr_held = !released;
        }
        return;
    }

    if (key == SC_LSHIFT || key == SC_RSHIFT) {
        shift_held = !released;
        return;
    }

    if (released || key >= KEYMAP_SIZE) {
        return;
    }

    char c;
    if (altgr_held)      c = altgr_map[key];
    else if (shift_held) c = shift_map[key];
    else                 c = normal_map[key];

    if (c != 0) {
        buffer_push(c);
    }
}

void keyboard_init(void) {
    irq_register(1, keyboard_irq);
    pic_unmask(1);
}

int keyboard_getchar(void) {
    if (tail == head) {
        return -1;              // nothing typed yet
    }
    char c = buffer[tail];
    tail = (tail + 1) % BUFFER_SIZE;
    return c;
}