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

// MSRs (model-specific registers) are CPU settings, read and written with
// the special rdmsr/wrmsr instructions instead of ports or memory.
static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t value) {
    asm volatile("wrmsr" :: "c"(msr), "a"((uint32_t)value), "d"((uint32_t)(value >> 32)));
}
#endif