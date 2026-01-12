; ==============================================================================
; The SakuraOS Kernel
; Copyright 2026 Adam Judge
; ==============================================================================

extern handle_exception

%define KERNEL_CS     0x0008
%define KERNEL_DS     0x0010
%define USER_CS       0x0018
%define USER_DS       0x020

%define GATE_INT32    0x8E
%define GATE_TRAP32   0x8F
%define GATE_RING3    0x60

%define PIC0_CMD      0x20
%define PIC0_DATA     0x21
%define PIC1_CMD      0xA0
%define PIC1_DATA     0xA1
%define PIC_EOI       0x20

struc gatedesc
    .offset_lo: resw 1
    .segment:   resw 1
    .resvd:     resb 1
    .type:      resb 1
    .offset_hi: resw 1
endstruc

; Set a specified interrupt gate.
%macro intgate 2
    mov byte [ebx + 8*%1 + gatedesc.type], GATE_INT32
    mov word [ebx + 8*%1 + gatedesc.segment], KERNEL_CS
    mov eax, %2
    mov word [ebx + 8*%1 + gatedesc.offset_lo], ax
    shr eax, 16
    mov word [ebx + 8*%1 + gatedesc.offset_hi], ax
%endmacro

; Set a specified trap gate.
%macro trapgate 2
    mov byte [ebx + 8*%1 + gatedesc.type], GATE_TRAP32 | GATE_RING3
    mov word [ebx + 8*%1 + gatedesc.segment], KERNEL_CS
    mov eax, %2
    mov word [ebx + 8*%1 + gatedesc.offset_lo], ax
    shr eax, 16
    mov word [ebx + 8*%1 + gatedesc.offset_hi], ax
%endmacro

; Define exception handler stub with dummy error code.
%macro exception 1
    push 0
    push %1
    jmp common_handler
%endmacro

; Define exception handler stub with error code pushed by the processor.
%macro exception_with_ecode 1
    push %1
    jmp common_handler
%endmacro

; ==============================================================================
; Interrupt Descriptor Table
; ==============================================================================

section .bss

alignb 8
idt:
    resq 256
idt_end:

section .rodata

align 4
idt_desc:
    dw idt_end - idt - 1
    dd idt

; ==============================================================================
; Interrupt Descriptor Table and PIC Setup
; ==============================================================================

section .text

pic_wait:
    mov cl, 255
.loop:
    loop .loop
    ret

global setup_idt
setup_idt:
    ; Reprogram the PICs to remap external interrupts to vectors starting at 32.
    mov al, 0x11
    out PIC0_CMD, al
    out PIC1_CMD, al
    call pic_wait

    mov al, 0x20
    out PIC0_DATA, al
    mov al, 0x28
    out PIC1_DATA, al
    call pic_wait

    mov al, 0x04
    out PIC0_DATA, al
    mov al, 0x02
    out PIC1_DATA, al
    call pic_wait

    mov al, 0x01
    out PIC0_DATA, al
    out PIC1_DATA, al
    call pic_wait

    mov al, 0x00
    out PIC0_DATA, al
    out PIC1_DATA, al
    call pic_wait

    ; Fill in IDT with gate descriptors.
    mov ebx, idt
    trapgate   0, division_error
    trapgate   1, debug_exception
    trapgate   3, breakpoint
    trapgate   4, overflow
    trapgate   5, bounds_check
    trapgate   6, invalid_opcode
    trapgate   7, no_coprocessor
    trapgate   8, double_fault
    trapgate   9, coprocessor_segment
    trapgate  10, invalid_tss
    trapgate  11, segment_not_present
    trapgate  12, stack_fault
    trapgate  13, general_protection
    trapgate  14, page_fault
    trapgate  16, coprocessor_error
    intgate   32, irq0
    intgate   33, irq1
    intgate   34, irq2
    intgate   35, irq3
    intgate   36, irq4
    intgate   37, irq5
    intgate   38, irq6
    intgate   39, irq7
    intgate   40, irq8
    intgate   41, irq9
    intgate   42, irq10
    intgate   43, irq11
    intgate   44, irq12
    intgate   45, irq13
    intgate   46, irq14
    intgate   47, irq15
    trapgate  64, int64
    trapgate  65, int65
    trapgate  66, int66
    trapgate  67, int67
    trapgate  68, int68
    trapgate  69, int69
    trapgate  70, int70
    trapgate  71, int71
    trapgate  72, int72
    trapgate  73, int73
    trapgate  74, int74
    trapgate  75, int75
    trapgate  76, int76
    trapgate  77, int77
    trapgate  78, int78
    trapgate  79, int79
    trapgate  128, int128

    lidt [idt_desc]
    ret

; ==============================================================================
; Exception Handlers
; ==============================================================================

; This common handler is called by each exception and interrupt handler stub
; after pushing the exception number and optional error code. Here we fill in
; the rest of an exception struct on the stack to save processor state before
; calling the actual exception handling code in C.
common_handler:
    pusha
    mov ax, KERNEL_DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Send end-of-interrupt command to the PIC(s) if needed.
    mov bl, [esp + 32]
    cmp bl, 32
    jl .skip_eoi
    cmp bl, 47
    jg .skip_eoi

    ; Determine whether to send the EOI command to both PICs.
    mov al, PIC_EOI
    cmp bl, 40
    jle .no_pic1
    out PIC1_CMD, al
.no_pic1:
    out PIC0_CMD, al

    ; Call C exception handling code.
.skip_eoi:
    call handle_exception

global iret_from_exception
iret_from_exception:
    cmp word [esp + 44], KERNEL_CS
    je .skip_user_ds
    mov ax, USER_DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
.skip_user_ds:
    popa
    add esp, 8 ; Discard eno and err from stack.
    iret

; Exception and interrupt handler stubs.
ignore:                 iret
division_error:         exception 0
debug_exception:        exception 1
non_maskable_int:       exception 2
breakpoint:             exception 3
overflow:               exception 4
bounds_check:           exception 5
invalid_opcode:         exception 6
no_coprocessor:         exception 7
double_fault:           exception_with_ecode 8
coprocessor_segment:    exception 9
invalid_tss:            exception_with_ecode 10
segment_not_present:    exception_with_ecode 11
stack_fault:            exception_with_ecode 12
general_protection:     exception_with_ecode 13
page_fault:             exception_with_ecode 14
coprocessor_error:      exception 16
irq0:                   exception 32
irq1:                   exception 33
irq2:                   exception 34
irq3:                   exception 35
irq4:                   exception 36
irq5:                   exception 37
irq6:                   exception 38
irq7:                   exception 39
irq8:                   exception 40
irq9:                   exception 41
irq10:                  exception 42
irq11:                  exception 43
irq12:                  exception 44
irq13:                  exception 45
irq14:                  exception 46
irq15:                  exception 47
int64:                  exception 64
int65:                  exception 65
int66:                  exception 66
int67:                  exception 67
int68:                  exception 68
int69:                  exception 69
int70:                  exception 70
int71:                  exception 71
int72:                  exception 72
int73:                  exception 73
int74:                  exception 74
int75:                  exception 75
int76:                  exception 76
int77:                  exception 77
int78:                  exception 78
int79:                  exception 79
int128:                 exception 128
