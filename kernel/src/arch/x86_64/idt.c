#include <stdint.h>
#include <stddef.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/gdt.h>
#include <lib/printf.h>

#define IDT_ENTRIES 256

// type_attr: P | DPL | 0 | gate type
#define GATE_INTERRUPT 0x8E //present, ring 0, 64 bit interrupt gate
#define GATE_TRAP 0x8F //same, but leaves interrupts enabled

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

