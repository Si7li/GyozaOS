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

// Tells the linker this file doesn't need an executable stack.
.section .note.GNU-stack, "", @progbits