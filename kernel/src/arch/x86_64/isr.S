// isr.S - where the CPU lands on an interrupt.
//
// Each exception has a tiny stub that makes the stack look the same,
// then jumps to isr_common, which saves registers and calls C.

.code64
.text

// For exceptions where the CPU does NOT push an error code.
.macro ISR_NO_ERR num
isr_stub_\num:
    pushq $0                // fake error code, so every frame looks the same
    pushq $\num             // vector number
    jmp isr_common
.endm

// For exceptions where the CPU DOES push an error code.
.macro ISR_ERR num
isr_stub_\num:
    pushq $\num             // vector number (the error code is already there)
    jmp isr_common
.endm

// The 32 CPU exceptions. Only 8, 10-14, 17, 21, 29 and 30 have an error code.
ISR_NO_ERR 0     // divide by zero
ISR_NO_ERR 1     // debug
ISR_NO_ERR 2     // non-maskable interrupt
ISR_NO_ERR 3     // breakpoint (int3)
ISR_NO_ERR 4     // overflow
ISR_NO_ERR 5     // bound range exceeded
ISR_NO_ERR 6     // invalid opcode (ud2)
ISR_NO_ERR 7     // device not available
ISR_ERR    8     // double fault
ISR_NO_ERR 9     // coprocessor segment overrun (old, unused)
ISR_ERR    10    // invalid TSS
ISR_ERR    11    // segment not present
ISR_ERR    12    // stack-segment fault
ISR_ERR    13    // general protection fault
ISR_ERR    14    // page fault
ISR_NO_ERR 15    // reserved
ISR_NO_ERR 16    // x87 floating-point error
ISR_ERR    17    // alignment check
ISR_NO_ERR 18    // machine check
ISR_NO_ERR 19    // SIMD floating-point error
ISR_NO_ERR 20    // virtualization exception
ISR_ERR    21    // control protection exception
ISR_NO_ERR 22    // reserved
ISR_NO_ERR 23    // reserved
ISR_NO_ERR 24    // reserved
ISR_NO_ERR 25    // reserved
ISR_NO_ERR 26    // reserved
ISR_NO_ERR 27    // reserved
ISR_NO_ERR 28    // hypervisor injection exception
ISR_ERR    29    // VMM communication exception
ISR_ERR    30    // security exception
ISR_NO_ERR 31    // reserved

// Hardware IRQs 0-15 from the PIC, remapped to 32-47. None has an error code.
ISR_NO_ERR 32    // IRQ 0:  timer (PIT)
ISR_NO_ERR 33    // IRQ 1:  keyboard
ISR_NO_ERR 34    // IRQ 2:  cascade (the slave PIC, never fires itself)
ISR_NO_ERR 35    // IRQ 3:  COM2
ISR_NO_ERR 36    // IRQ 4:  COM1
ISR_NO_ERR 37    // IRQ 5:  LPT2 / free
ISR_NO_ERR 38    // IRQ 6:  floppy
ISR_NO_ERR 39    // IRQ 7:  LPT1 / spurious
ISR_NO_ERR 40    // IRQ 8:  real-time clock
ISR_NO_ERR 41    // IRQ 9:  free
ISR_NO_ERR 42    // IRQ 10: free
ISR_NO_ERR 43    // IRQ 11: free
ISR_NO_ERR 44    // IRQ 12: PS/2 mouse
ISR_NO_ERR 45    // IRQ 13: FPU
ISR_NO_ERR 46    // IRQ 14: primary ATA disk
ISR_NO_ERR 47    // IRQ 15: secondary ATA disk / spurious

// Shared by every stub. Saves the 15 registers, calls C, restores, returns.
isr_common:
    // Push order must match interrupt_frame_t (reversed) in idt.h.
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %rsp, %rdi         // 1st C argument = pointer to the frame
    cld                     // C code expects this flag cleared
    call interrupt_handler

    // Restore in exactly the reverse order.
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax

    addq $16, %rsp          // throw away vector + error code
    iretq                   // pop the CPU's 5 values and resume

// Addresses of the 32 stubs, so idt.c can put them in the table.
.section .rodata
.global isr_stub_table
isr_stub_table:
    .quad isr_stub_0,  isr_stub_1,  isr_stub_2,  isr_stub_3
    .quad isr_stub_4,  isr_stub_5,  isr_stub_6,  isr_stub_7
    .quad isr_stub_8,  isr_stub_9,  isr_stub_10, isr_stub_11
    .quad isr_stub_12, isr_stub_13, isr_stub_14, isr_stub_15
    .quad isr_stub_16, isr_stub_17, isr_stub_18, isr_stub_19
    .quad isr_stub_20, isr_stub_21, isr_stub_22, isr_stub_23
    .quad isr_stub_24, isr_stub_25, isr_stub_26, isr_stub_27
    .quad isr_stub_28, isr_stub_29, isr_stub_30, isr_stub_31
    .quad isr_stub_32, isr_stub_33, isr_stub_34, isr_stub_35
    .quad isr_stub_36, isr_stub_37, isr_stub_38, isr_stub_39
    .quad isr_stub_40, isr_stub_41, isr_stub_42, isr_stub_43
    .quad isr_stub_44, isr_stub_45, isr_stub_46, isr_stub_47

// Tells the linker this file doesn't need an executable stack.
.section .note.GNU-stack, "", @progbits