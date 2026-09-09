#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <lib/printf.h>

#define MAX_SINKS 4

static printf_sink_t sinks[MAX_SINKS];
static size_t sink_count;

void kprintf_add_sink(printf_sink_t sink){
    if (sink_count < MAX_SINKS){
        sinks[sink_count++] = sink;
    }
}

static void broadcast(char c){
    for(size_t i = 0; i < sink_count; i++){
        sinks[i](c);
    }
}

static int emit_str(printf_sink_t sink, const char *s){
    int n = 0;
    while( *s != '\0'){
        sink(*s++);
        n++;
    }
    return n;
}

// Digits come out least-significant first so we fill a buffer backwards
// emit the tail. 64 digits is the worst case (a uint64_t printed in base 2)

static int emit_uint(printf_sink_t sink, uint64_t v, unsigned base, bool upper){
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char buf[65];
    size_t i = sizeof(buf);

    buf[--i] = '\0';
    do{
        buf[--i] = digits[v % base];
        v /= base;
    } while(v!=0);
    return emit_str(sink, &buf[i]);   
}

static int emit_int(printf_sink_t sink, int64_t v, unsigned base){
    int n = 0;
    uint64_t mag;

    if(v<0){
        sink('-');
        n++;
        mag = -(uint64_t)v;
    }else{
        mag = (uint64_t)v;
    }

    return n + emit_uint(sink, mag, base, false);
}

// Kernel pointers are always printed full width so addresses line up visually.
static int emit_ptr(printf_sink_t sink, uintptr_t p) {
    int n = emit_str(sink, "0x");
    for (int shift = 60; shift >= 0; shift -= 4) {
        sink("0123456789abcdef"[(p >> shift) & 0xF]);
        n++;
    }
    return n;
}
int printf_format(printf_sink_t sink, const char *fmt, va_list ap) {
    int n = 0;

    for (; *fmt != '\0'; fmt++) {
        if (*fmt != '%') {
            sink(*fmt);
            n++;
            continue;
        }

        fmt++;
        bool alt = false;
        while (*fmt=='#')
        {
            alt = true;
            fmt++;
        }
        
        // Length modifiers. 'l' and 'll' both mean 64-bit here because long is
        // 64-bit on x86-64; 'z' covers size_t.
        bool is_long = false;
        while (*fmt == 'l') {
            is_long = true;
            fmt++;
        }
        if (*fmt == 'z') {
            is_long = true;
            fmt++;
        }

        switch (*fmt) {
        case '\0':
            // Trailing '%' at end of string: stop before the loop's fmt++
            // walks us off the end of the buffer.
            return n;

        case 'c':
            // Must be int, not char: C promotes small types passed through ...
            sink((char)va_arg(ap, int));
            n++;
            break;

        case 's': {
            const char *s = va_arg(ap, const char *);
            n += emit_str(sink, s != NULL ? s : "(null)");
            break;
        }

        case 'd':
        case 'i':
            n += emit_int(sink, is_long ? va_arg(ap, int64_t)
                                        : (int64_t)va_arg(ap, int32_t), 10);
            break;

        case 'u':
            n += emit_uint(sink, is_long ? va_arg(ap, uint64_t)
                                         : (uint64_t)va_arg(ap, uint32_t),
                           10, false);
            break;

        case 'x':
        case 'X': {
            uint64_t v = is_long ? va_arg(ap, uint64_t)
                                 : (uint64_t)va_arg(ap, uint32_t);
            bool upper = (*fmt == 'X');
            if (alt && v != 0) {
                n += emit_str(sink, upper ? "0X" : "0x");
            }
            n += emit_uint(sink, v, 16, upper);
            break;
        }

        case 'p':
            n += emit_ptr(sink, (uintptr_t)va_arg(ap, void *));
            break;

        case '%':
            sink('%');
            n++;
            break;

        default:
            // Unknown specifier: show it literally rather than silently
            // eating an argument and desynchronising everything after it.
            sink('%');
            sink(*fmt);
            n += 2;
            break;
        }
    }

    return n;
}

int kvprintf(const char *fmt, va_list ap) {
    return printf_format(broadcast, fmt, ap);
}

int kprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = kvprintf(fmt, ap);
    va_end(ap);
    return n;
}