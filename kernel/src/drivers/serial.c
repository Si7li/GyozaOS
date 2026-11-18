#include <stdint.h>
#include <stdbool.h>
#include <arch/x86_64/io.h>
#include <drivers/serial.h>

// COM1. The other legacy ports are 0x2F8 (COM2), 0x3E8 (COM3), 0x2E8 (COM4).
#define COM1_BASE 0x3F8

// Register offsets from the base port. Offsets 0 and 1 change meaning
// depending on the DLAB bit (bit 7 of LCR) -- see the divisor writes below.
#define SERIAL_DATA 0  // RX/TX buffer     (divisor low byte  when DLAB=1)
#define SERIAL_IER  1  // interrupt enable (divisor high byte when DLAB=1)
#define SERIAL_FCR  2  // FIFO control     (write-only; reads give IIR)
#define SERIAL_LCR  3  // line control
#define SERIAL_MCR  4  // modem control
#define SERIAL_LSR  5  // line status

#define LCR_DLAB      0x80  // divisor latch access bit
#define LCR_8N1       0x03  // 8 data bits, no parity, 1 stop bit
#define MCR_NORMAL    0x0F  // DTR + RTS + OUT1 + OUT2
#define MCR_LOOPBACK  0x1E  // same, but loop TX back into RX
#define LSR_THR_EMPTY 0x20  // transmit holding register is free

// The UART is clocked at 115200 Hz; baud = 115200 / divisor.
#define BAUD_DIVISOR 1

static bool transmit_empty(void) {
    return (inb(COM1_BASE + SERIAL_LSR) & LSR_THR_EMPTY) != 0;
}

int serial_init(void) {
    outb(COM1_BASE + SERIAL_IER, 0x00);          // no interrupts, we poll
    outb(COM1_BASE + SERIAL_LCR, LCR_DLAB);      // offsets 0/1 are now the divisor
    outb(COM1_BASE + SERIAL_DATA, BAUD_DIVISOR & 0xFF);
    outb(COM1_BASE + SERIAL_IER, (BAUD_DIVISOR >> 8) & 0xFF);
    outb(COM1_BASE + SERIAL_LCR, LCR_8N1);       // clears DLAB, sets 8N1
    outb(COM1_BASE + SERIAL_FCR, 0xC7);          // enable + clear FIFOs, 14-byte trigger
    outb(COM1_BASE + SERIAL_MCR, MCR_NORMAL);

    // Self-test: in loopback mode whatever we transmit comes straight back.
    // If it doesn't, there is no usable UART here and callers should know.
    outb(COM1_BASE + SERIAL_MCR, MCR_LOOPBACK);
    outb(COM1_BASE + SERIAL_DATA, 0xAE);
    if (inb(COM1_BASE + SERIAL_DATA) != 0xAE) {
        return 0;
    }

    outb(COM1_BASE + SERIAL_MCR, MCR_NORMAL);
    return 1;
}

void serial_putc(char c) {
    // Terminals descend from teletypes: line feed moves down a row, carriage
    // return moves to column 0. Send only '\n' and output staircases rightward.
    if (c == '\n') {
        serial_putc('\r');
    }

    while (!transmit_empty()) {
    }

    outb(COM1_BASE + SERIAL_DATA, (uint8_t)c);
}

void serial_puts(const char *s) {
    while (*s != '\0') {
        serial_putc(*s++);
    }
}