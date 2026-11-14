#include <stdint.h>
#include <stddef.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/gdt.h>
#include <lib/printf.h>
#include <arch/x86_64/cpu.h>
#include <arch/x86_64/pic.h>

#define IDT_ENTRIES 256

// type_attr: P | DPL | 0 | gate type
#define GATE_INTERRUPT 0x8E //present, ring 0, 64 bit interrupt gate
#define GATE_TRAP 0x8F //same, but leaves interrupts enabled

// kprintf's %p always prints 16 hex digits, which keeps the columns lined up.
#define HEX(x) ((void *)(x))

// a gate descriptor.
// historically three fields : the 32-bit IDT entry already used offset_low and offser_mid, so long mode bolted offset_high on the end.
typedef struct __attribute__((packed)){
    uint16_t offset_low; //offset bits 0..15
    uint16_t selector; //a code segment in GDT or LDT
    uint8_t ist; //bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
    uint8_t type_attr; //gate type, dpl, and p fields
    uint16_t offset_mid; //offset bits 16..31
    uint32_t offset_high; //offset bits 32..63
    uint32_t reserved; //reserved
} idt_entry_t;

// The operand lidt reads same shape as the GDTR
typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} idtr_t;

_Static_assert(sizeof(idt_entry_t) == 16, "IDT gate must be 16 bytes");

// Human-readable names for the 32 CPU exceptions, indexed by vector number.
static const char *exception_names[32] = {
    "Divide by zero",           "Debug",
    "Non-maskable interrupt",   "Breakpoint",
    "Overflow",                 "Bound range exceeded",
    "Invalid opcode",           "Device not available",
    "Double fault",             "Coprocessor segment overrun",
    "Invalid TSS",              "Segment not present",
    "Stack-segment fault",      "General protection fault",
    "Page fault",               "Reserved",
    "x87 floating-point error", "Alignment check",
    "Machine check",            "SIMD floating-point error",
    "Virtualization exception", "Control protection exception",
    "Reserved",                 "Reserved",
    "Reserved",                 "Reserved",
    "Reserved",                 "Reserved",
    "Hypervisor injection",     "VMM communication",
    "Security exception",       "Reserved",
};

// One slot per hardware IRQ. NULL means "nobody handles this one yet".
static irq_handler_t irq_handlers[16];

void irq_register(uint8_t irq, irq_handler_t handler) {
    if (irq < 16) {
        irq_handlers[irq] = handler;
    }
}

// Called by isr_common in isr.S for every interrupt.
void interrupt_handler(interrupt_frame_t *f) {
    // Hardware IRQs (32-47): run the driver's function, then tell the PIC we're done.
    if (f->vector >= PIC_IRQ_BASE && f->vector < PIC_IRQ_BASE + 16) {
        uint8_t irq = f->vector - PIC_IRQ_BASE;
        if (irq_handlers[irq] != NULL) {
            irq_handlers[irq](f);
        }
        pic_send_eoi(irq);
        return;
    }
    const char *name = f->vector < 32 ? exception_names[f->vector] : "Unknown";

    // Breakpoint (int3) is harmless: report it and go back to the code.
    if (f->vector == 3) {
        kprintf("breakpoint hit at %p, continuing\n", HEX(f->rip));
        return;
    }

    kprintf("\n\e[97;41m KERNEL PANIC \e[0m %s (vector %lu, error code %#lx)\n",
            name, f->vector, f->error_code);

    // For a page fault, CR2 holds the address the code tried to touch.
    if (f->vector == 14) {
        kprintf("  bad address (CR2): %p\n", HEX(read_cr2()));
    }

    kprintf("  RIP %p   RSP %p   RFLAGS %p\n", HEX(f->rip), HEX(f->rsp), HEX(f->rflags));
    kprintf("  CS  %p   SS  %p\n", HEX(f->cs), HEX(f->ss));
    kprintf("  RAX %p   RBX %p   RCX %p\n", HEX(f->rax), HEX(f->rbx), HEX(f->rcx));
    kprintf("  RDX %p   RSI %p   RDI %p\n", HEX(f->rdx), HEX(f->rsi), HEX(f->rdi));
    kprintf("  RBP %p   R8  %p   R9  %p\n", HEX(f->rbp), HEX(f->r8),  HEX(f->r9));
    kprintf("  R10 %p   R11 %p   R12 %p\n", HEX(f->r10), HEX(f->r11), HEX(f->r12));
    kprintf("  R13 %p   R14 %p   R15 %p\n", HEX(f->r13), HEX(f->r14), HEX(f->r15));

    halt_forever();
}

// The table itself: 256 gates.  We fill the first 48: 32 exceptions + 16 IRQs for now.
static idt_entry_t idt[IDT_ENTRIES] __attribute__((aligned(16)));
static idtr_t      idtr;

// The 48 stub addresses, defined in isr.S.
extern uint64_t isr_stub_table[48];

// Fill one gate so that interrupt `vector` jumps to `handler`.
static void set_gate(int vector, uint64_t handler) {
    idt_entry_t *e = &idt[vector];

    e->offset_low  = (uint16_t)handler;           // address bits 0-15
    e->selector    = GDT_KERNEL_CODE;             // run the handler as kernel code
    e->ist         = 0;                           // use the current stack
    e->type_attr   = GATE_INTERRUPT;              // present, ring 0, interrupt gate
    e->offset_mid  = (uint16_t)(handler >> 16);   // address bits 16-31
    e->offset_high = (uint32_t)(handler >> 32);   // address bits 32-63
    e->reserved    = 0;
}

void idt_init(void) {
    for (int v = 0; v < 48; v++) {
        set_gate(v, isr_stub_table[v]);
    }

    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)&idt;

    asm volatile("lidt %0" :: "m"(idtr) : "memory");
}