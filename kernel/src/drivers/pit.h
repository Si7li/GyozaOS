#ifndef PIT_H
#define PIT_H

#include <stdint.h>

void     pit_init(uint32_t hz);     // start ticking `hz` times per second (19 or more)
uint64_t pit_ticks(void);           // ticks since pit_init
void     pit_sleep_ms(uint64_t ms); // wait; needs interrupts turned on

#endif