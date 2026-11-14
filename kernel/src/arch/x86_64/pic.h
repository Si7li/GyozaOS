#ifndef PIC_H
#define PIC_H

#include <stdint.h>

// After pic_init(), IRQ n arrives as interrupt number PIC_IRQ_BASE + n.
// So the timer (IRQ 0) is 32 and the keyboard (IRQ 1) is 33.
// Basically remapping
#define PIC_IRQ_BASE 32

void pic_init(void);                // remap to 32-47 and mask every IRQ
void pic_unmask(uint8_t irq);       // let one IRQ through
void pic_send_eoi(uint8_t irq);     // tell the PIC we finished handling an IRQ

#endif