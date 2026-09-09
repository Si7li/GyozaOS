#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>

// Where formatted output goes. Anything that can accept one character at a
// time can be a sink: the serial port, the framebuffer console, a log ring.
typedef void (*printf_sink_t)(char c);

// Register an output. Every subsequent kprintf goes to all registered sinks.
void kprintf_add_sink(printf_sink_t sink);

// The core formatter. Emits through the given sink only; nothing else in the
// kernel needs this, but it is what makes the module reusable.
int printf_format(printf_sink_t sink, const char *fmt, va_list ap);

// Formatted output to all registered sinks. Both return the character count.
int kvprintf(const char *fmt, va_list ap);
int kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif