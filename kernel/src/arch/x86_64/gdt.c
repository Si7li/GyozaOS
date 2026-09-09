#include <stdint.h>
#include <stddef.h>
#include <arch/x86_64/gdt.h>

// ACcess byte: P / DPL / S / Type

#define ACCESS_KERNEL_CODE 0x9A //present, ring 0, code, readable
#define ACCESS_KERNEL_DATA 0x92 //present, ring 0, data, writable
#define ACCESS_USER_CODE   0xFA //present, ring 3, code, readable
#define ACCESS_USER_DATA   0xF2 //present, ring 3, data, writable
#define ACCESS_TSS         0x89 //present, ring 0, system, 64-bit TSS

// Granularity byte, bit 5 is the L flag:
// L: Long-mode code flag. If set (1), 
//the descriptor defines a 64-bit code segment.
// When set, DB should always be clear. 
//For any other type of segment (other code types or any data segment), 
//it should be clear (0).
#define GRAN_LONG_MODE     0x20

// An 8-byte segment descriptor. In long mode the CPU ignores base and limit
// for code and data segments -- enery segment us implicitly flat. Only the
// access byte and the L flah still mean anything.
typedef struct __attribute__((packed)) {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} gdt_entry_t;

// A system descriptor is 16 bytes, not 8: it needs room for a 64-bit base.
typedef struct __attribute__((packed)) {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
    uint32_t base_upper;
    uint32_t reserved;
} tss_desc_t;

// The operand lgdt reads: a 2-byte limit followed by an 8-byte base.
typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} gdtr_t;

// Long mode dropped hardware task switching, so almost all of this is dead
// weight. It survives for rsp0 -- the stack the CPU switches to on a
// ring 3 -> ring 0 transition -- and the IST alternate fault stacks.
typedef struct __attribute__((packed)) {
    uint32_t reserved0;
    uint64_t rsp[3];
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} tss_t;

typedef struct __attribute__((packed)) {
    gdt_entry_t entries[5];
    tss_desc_t  tss_desc;
} gdt_table_t;

_Static_assert(sizeof(gdt_entry_t) == 8,  "segment descriptor must be 8 bytes");
_Static_assert(sizeof(tss_desc_t)  == 16, "system descriptor must be 16 bytes");
_Static_assert(sizeof(gdtr_t)      == 10, "GDTR must be 10 bytes");
_Static_assert(sizeof(tss_t)       == 104, "64-bit TSS must be 104 bytes");
_Static_assert(offsetof(gdt_table_t, tss_desc) == GDT_TSS,
               "TSS descriptor is not at the offset gdt.h advertises");

// Zero-initialised: these live in .bss, which the linker script clears.
static gdt_table_t gdt __attribute__((aligned(16)));
static tss_t       tss __attribute__((aligned(16)));
static gdtr_t      gdtr;

static void set_entry(gdt_entry_t *e, uint8_t access, uint8_t granularity) {
    e->limit_low   = 0;     // ignored in long mode
    e->base_low    = 0;
    e->base_mid    = 0;
    e->access      = access;
    e->granularity = granularity;
    e->base_high   = 0;
}

static void set_tss_desc(tss_desc_t *d, const tss_t *t) {
    uint64_t addr = (uint64_t)t;

    d->limit_low   = sizeof(tss_t) - 1;   // limit is size - 1
    d->base_low    = (uint16_t)addr;
    d->base_mid    = (uint8_t)(addr >> 16);
    d->access      = ACCESS_TSS;
    d->granularity = 0;
    d->base_high   = (uint8_t)(addr >> 24);
    d->base_upper  = (uint32_t)(addr >> 32);
    d->reserved    = 0;
}

static void gdt_flush(void) {
    asm volatile(
        "lgdt %0\n\t"
        // CS cannot be loaded with mov. Fake a far return instead: push the
        // new selector and a return address, then lretq pops both into CS:RIP.
        // Execution resumes on the very next line, but under our descriptor.
        "pushq %1\n\t"
        "leaq 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "lretq\n\t"
        "1:\n\t"
        // Data segments can be loaded directly.
        // NOTE: writing to GS zeroes GS.base. Harmless now, but at Milestone 8
        // GS.base holds the per-CPU pointer -- do not repeat this after that.
        "movl %2, %%eax\n\t"
        "movl %%eax, %%ds\n\t"
        "movl %%eax, %%es\n\t"
        "movl %%eax, %%fs\n\t"
        "movl %%eax, %%gs\n\t"
        "movl %%eax, %%ss\n\t"
        :
        : "m"(gdtr), "i"((uint64_t)GDT_KERNEL_CODE), "i"(GDT_KERNEL_DATA)
        : "rax", "memory");
}
void gdt_init(void) {
    set_entry(&gdt.entries[0], 0,                  0);                // null
    set_entry(&gdt.entries[1], ACCESS_KERNEL_CODE, GRAN_LONG_MODE);
    set_entry(&gdt.entries[2], ACCESS_KERNEL_DATA, 0);
    set_entry(&gdt.entries[3], ACCESS_USER_DATA,   0);                // before
    set_entry(&gdt.entries[4], ACCESS_USER_CODE,   GRAN_LONG_MODE);   // code

    // An I/O permission bitmap offset at or past the segment limit means
    // "no bitmap", so ring 3 gets no port access.
    tss.iopb_offset = sizeof(tss_t);
    set_tss_desc(&gdt.tss_desc, &tss);

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (uint64_t)&gdt;

    gdt_flush();

    asm volatile("ltr %0" :: "rm"((uint16_t)GDT_TSS) : "memory");
}