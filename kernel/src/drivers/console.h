#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stddef.h>

void console_init();

void console_clear(void);
// Set the colours used for subsequent output, format is 0x00RRGGBB
void console_set_color(uint32_t fg, uint32_t bg);

// Write one character at the cursor, handling newlines, wrapping and scrolling
// Signature matches printf_sink_t so it can be registered with kprintf_add_sink() directrly
void console_putc(char c);

#endif
