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
#define SC_RELEASE  0x80    // bit 7 set = key released
#define SC_EXTENDED 0xE0    // prefix for arrows, right ctrl, etc.

// Scancode -> character, US layout. 0 = no character (ctrl, shift, alt...).
// The position in the array is the scancode: index 0x1E is the 'a' key.
static const char normal_map[58] = {
    0,    27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*',  0,   ' ',
};

static const char shift_map[58] = {
    0,    27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*',  0,   ' ',
};

// A small circular buffer between the interrupt (which adds characters)
// and the rest of the kernel (which takes them out).
#define BUFFER_SIZE 128
static volatile char     buffer[BUFFER_SIZE];
static volatile uint32_t head;   // where the interrupt writes next
static volatile uint32_t tail;   // where keyboard_getchar reads next

static bool shift_held;
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

    // Arrows and friends send 0xE0 first. Skip them for now.
    if (sc == SC_EXTENDED) {
        extended = true;
        return;
    }
    if (extended) {
        extended = false;
        return;
    }

    bool released = sc & SC_RELEASE;
    uint8_t key   = sc & ~SC_RELEASE;

    if (key == SC_LSHIFT || key == SC_RSHIFT) {
        shift_held = !released;
        return;
    }

    if (released || key >= sizeof(normal_map)) {
        return;
    }

    char c = shift_held ? shift_map[key] : normal_map[key];
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