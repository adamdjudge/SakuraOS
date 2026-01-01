; ==============================================================================
; The SakuraOS Bootloader
; Copyright 2026 Adam Judge
; ==============================================================================

; Memory range descriptor returned by each 0xE820 BIOS call
struc memrange
    .addr:    resq 1
    .length:  resq 1
    .type:    resd 1
endstruc

ENTRY_SIZE    equ 20

section .text.start
bits 16

; Use BIOS memory detection calls to fill in the table of memory ranges
global memdetect
memdetect:
    mov dword [memtab + memrange.length], 0
    call detect_e820
    ret

; Try using 0xE820 BIOS call
; TODO: fall back to 0xE801 and 0x88 calls if this fails
detect_e820:
    xor ebx, ebx
    mov di, memtab
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
    cmp dword [memtab + memrange.length], 0
    je .err
    clc
    ret
.err:
    stc
    ret

section .bss
alignb 4

global memtab
memtab:
    resb 16 * ENTRY_SIZE
