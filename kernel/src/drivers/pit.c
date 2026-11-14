#include <stdint.h>
#include <arch/x86_64/io.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/pic.h>
#include <drivers/pit.h>

#define PIT_CHANNEL0 0x40       // channel 0's counter; it is wired to IRQ 0
#define PIT_COMMAND  0x43       // the PIT's settings port
#define PIT_BASE_HZ  1193182    // the PIT's clock: it counts down this many times a second

// volatile: changed by the interrupt, so the compiler must re-read it every time.
static volatile uint64_t ticks;
static uint32_t tick_hz;

// Runs on every IRQ 0.
static void pit_irq(interrupt_frame_t *frame) {
    (void)frame;    // not needed here
    ticks++;
}

void pit_init(uint32_t hz) {
    // The PIT fires once every `divisor` counts. It is a 16-bit number,
    // so hz must be at least 19 (1193182 / 65535).
    uint32_t divisor = PIT_BASE_HZ / hz;
    tick_hz = hz;

    // 0x36 = channel 0, send the divisor as low byte then high byte,
    // mode 3 (repeat forever).
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);

    irq_register(0, pit_irq);
    pic_unmask(0);
}

uint64_t pit_ticks(void) {
    return ticks;
}

void pit_sleep_ms(uint64_t ms) {
    uint64_t end = ticks + (ms * tick_hz) / 1000;
    while (ticks < end) {
        asm volatile("hlt");    // sleep until the next interrupt instead of spinning
    }
}