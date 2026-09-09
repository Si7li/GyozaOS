#ifndef CPU_H
#define CPU_H

#include <stdint.h>

// Control registers

static inline uint64_t read_cr0(void){
    uint64_t v;
    asm volatile ("mov %%cr0, %0" : "=r"(v));
    return v;
}

// THe linear address that caused the most recent page fault.
// The single most useful register in kernel debugging.
static inline uint64_t read_cr2(void){
    uint64_t v;
    asm volatile ("mov %%cr2, %0" : "=r"(v));
    return v;
}

// Physical address of the top-level page table.
static inline uint64_t read_cr3(void){
    uint64_t v;
    asm volatile ("mov %%cr3, %0" : "=r"(v));
    return v;
}

static inline uint64_t read_cr4(void){
    uint64_t v;
    asm volatile ("mov %%cr4, %0": "=r"(v));
    return v;
}

static inline void write_cr0(uint64_t v){
    asm volatile ("mov %0, %%cr0" :: "r"(v) : "memory");
}

static inline void write_cr3(uint64_t v){
    asm volatile ("mov %0, %%cr3" :: "r"(v) : "memory");
}

static inline void write_cr4(uint64_t v){
    asm volatile ("mov %0, %%cr4" :: "r"(v) : "memory");
}

static inline void cli(void){
    asm volatile ("cli" ::: "memory");
}

static inline void sti(void){
    asm volatile("sti" ::: "memory");
}

//Stop until the next interrupt. With interrupts disabled forever.
static inline void hlt(void){
    asm volatile ("hlt");
}

// Halt and catch fire. Same as the hcf() in main.c but reachable from anywhare the panic handler will want it.
static inline __attribute__((noreturn)) void halt_forever(void){
    for(;;){
        asm volatile ("cli; hlt");
    }
}
#endif