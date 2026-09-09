#ifndef GDT_H
#define GDT_H

// Segment selectors: byte offsets into the GDT.
//
// The order is not arbitrary. syscall/sysret do not take a selector operand;
// they derive their segments from the STAR MSR by fixed offsets:
//
//   syscall:  CS = STAR[47:32],      SS = STAR[47:32] + 8
//   sysret:   SS = STAR[63:48] + 8,  CS = STAR[63:48] + 16
//
// So kernel code must be immediately followed by kernel data, and user data
// must come BEFORE user code. With this layout, Milestone 8 sets
// STAR[47:32] = 0x08 and STAR[63:48] = 0x10 and the arithmetic works out.
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_DATA   0x18
#define GDT_USER_CODE   0x20
#define GDT_TSS         0x28

// Build the GDT and TSS, load them, and reload every segment register.
void gdt_init(void);


#endif