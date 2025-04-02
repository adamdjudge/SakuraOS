; ==============================================================================
; The SakuraOS Kernel
; Copyright 2025 Adam Judge
; ==============================================================================

extern early_panic

; Memory range descriptor returned by each 0xE820 BIOS call
struc memrange
    .addr:    resq 1
    .length:  resq 1
    .type:    resd 1
endstruc

ENTRY_SIZE    equ 20

section .text
bits 16

; Use BIOS memory detection calls to fill in the table of memory ranges
global detect_memory
detect_memory:
    call detect_e820
    jnc .done
    mov si, mem_panic_msg
    call early_panic
.done:
    ret

; Try using 0xE820 BIOS call
; TODO: fall back to 0xE801 and 0x88 calls if this fails
detect_e820:
    xor ebx, ebx
    mov di, g_memory_table
.loop:
    mov eax, 0xE820
    mov ecx, ENTRY_SIZE
    mov edx, 'PAMS'
    int 0x15
    jc .done
    cmp eax, 'PAMS'
    jne .err
    cmp ecx, ENTRY_SIZE
    jne .err
    cmp ebx, 0
    je .done
    add di, ENTRY_SIZE
    jmp .loop
.done:
    cmp dword [g_memory_table + memrange.length], 0
    je .err
    clc
    ret
.err:
    stc
    ret

section .rodata
alignb 4

global g_memory_table
g_memory_table:
    resb 16 * ENTRY_SIZE

section .rodata

mem_panic_msg:
    db "failed to detect memory layout", 0
