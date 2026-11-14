#ifndef IDT_H
#define IDT_H

#include <stdint.h>

typedef struct  __attribute__((packed)){
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;

    uint64_t vector;      // which interrupt (0-255)
    uint64_t error_code;  // real one from the CPU, or a fake 0

    // Pushed by the CPU automatically.
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} interrupt_frame_t;

// 22 values x 8 bytes. If this fails, a field was added or lost.
_Static_assert(sizeof(interrupt_frame_t) == 176, "interrupt frame must be 176 bytes");

void idt_init(void);
// A driver's function for one hardware IRQ (0-15).
typedef void (*irq_handler_t)(interrupt_frame_t *frame);
// Ask for `handler` to be called whenever `irq` fires.
void irq_register(uint8_t irq, irq_handler_t handler);
#endif

