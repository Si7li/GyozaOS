#include <stdint.h>
#include <arch/x86_64/io.h>
#include <arch/x86_64/pic.h>
#include <arch/x86_64/cpu.h>

// There are two PICs chained together, each with a command port and a data port.
#define PIC1_CMD  0x20   // master: IRQs 0-7
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0   // slave: IRQs 8-15
#define PIC2_DATA 0xA1

#define PIC_EOI   0x20   // the "end of interrupt" command

#define MSR_APIC_BASE      0x1B
#define APIC_GLOBAL_ENABLE (1ULL << 11)

// Old PICs need a short pause between writes. Writing to the unused
// port 0x80 takes just long enough.
static inline void io_wait(void) {
    outb(0x80, 0);
}

void pic_init(void) {
    // Limine leaves the Local APIC on, with its input from the PIC blocked.
    // Turning the Local APIC off makes the PIC talk to the CPU directly again.
    wrmsr(MSR_APIC_BASE, rdmsr(MSR_APIC_BASE) & ~APIC_GLOBAL_ENABLE);
    // 1. Start the setup sequence on both PICs.
    //    0x11 = "begin init, and I will send all 4 setup bytes".
    outb(PIC1_CMD, 0x11); io_wait();
    outb(PIC2_CMD, 0x11); io_wait();

    // 2. The new interrupt numbers. This is the actual remap.
    outb(PIC1_DATA, PIC_IRQ_BASE);     io_wait();   // master: 32-39
    outb(PIC2_DATA, PIC_IRQ_BASE + 8); io_wait();   // slave:  40-47

    // 3. How the two PICs are wired: the slave is plugged into master's IRQ 2.
    outb(PIC1_DATA, 0x04); io_wait();   // master: "slave is on line 2" (bit 2 set)
    outb(PIC2_DATA, 0x02); io_wait();   // slave:  "you are on line 2"

    // 4. Mode: 0x01 = normal x86 mode.
    outb(PIC1_DATA, 0x01); io_wait();
    outb(PIC2_DATA, 0x01); io_wait();

    // Block every IRQ for now. Each one gets unmasked once it has a handler.
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void pic_unmask(uint8_t irq) {
    if (irq >= 8) {
        // Slave IRQs reach the CPU through master's IRQ 2, so open that too.
        pic_unmask(2);
        outb(PIC2_DATA, inb(PIC2_DATA) & (uint8_t)~(1 << (irq - 8)));
    } else {
        outb(PIC1_DATA, inb(PIC1_DATA) & (uint8_t)~(1 << irq));
    }
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);   // an IRQ from the slave went through both PICs
    }
    outb(PIC1_CMD, PIC_EOI);
}