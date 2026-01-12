; ==============================================================================
; The SakuraOS Kernel
; Copyright 2026 Adam Judge
; ==============================================================================

extern __kernel_bss
extern __kernel_end
extern setup_idt
extern kmain
extern idle

%define KERNEL_CS 0x08
%define KERNEL_DS 0x10
%define KERNEL_TS 0x28
%define TSS_SIZE  108

struc gdtentry
    .limit_lo:       resw 1
    .base_lo:        resb 3
    .access:         resb 1
    .flags_limit_hi: resb 1
    .base_hi:        resb 1
endstruc

; ==============================================================================
; Entry point from bootloader.
; At this point the bootloader has already enabled A20 and entered protected
; mode, but we need to install a new GDT with user and TSS segments.
; ==============================================================================

section .text
bits 32

global _start
_start:
    lgdt [gdt_desc]
    jmp KERNEL_CS:.new_gdt
.new_gdt:
    mov ax, KERNEL_DS
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Zero out the BSS.
    mov edi, __kernel_bss
    mov ecx, __kernel_end
    sub ecx, __kernel_end
    shr ecx, 2
    add ecx, 1
    xor eax, eax
    rep stosd

    ; Set up TSS.
    mov word [gdt + KERNEL_TS + gdtentry.limit_lo], TSS_SIZE - 1
    mov dword [gdt + KERNEL_TS + gdtentry.base_lo], tss
    mov byte [gdt + KERNEL_TS + gdtentry.access], 0x89
    mov word [tss_ss0], KERNEL_DS
    mov ax, KERNEL_TS
    ltr ax

    ; Get CPUID processor info.
    xor eax, eax
    cpuid
    mov [cpuid_vendor], ebx
    mov [cpuid_vendor+4], edx
    mov [cpuid_vendor+8], ecx

    mov eax, 0x80000002
    cpuid
    mov [cpuid_brand], eax
    mov [cpuid_brand+4], ebx
    mov [cpuid_brand+8], ecx
    mov [cpuid_brand+12], edx

    mov eax, 0x80000003
    cpuid
    mov [cpuid_brand+16], eax
    mov [cpuid_brand+20], ebx
    mov [cpuid_brand+24], ecx
    mov [cpuid_brand+28], edx

    mov eax, 0x80000004
    cpuid
    mov [cpuid_brand+32], eax
    mov [cpuid_brand+36], ebx
    mov [cpuid_brand+40], ecx
    mov [cpuid_brand+44], edx

    mov eax, 0x16
    cpuid
    mov [cpuid_base_freq], ax

    ; Set up the IDT and call kernel main function.
    call setup_idt
    call kmain

    ; After initialization, this thread becomes the idle thread.
    mov esp, idle + 1024
    sti
.idle:
    hlt
    jmp .idle

; ==============================================================================
; Processor Data Structures
; ==============================================================================

section .rodata
align 8

; Global Descriptor Table
gdt:
    dq 0x0000_0000_0000_0000  ; Null segment
    dq 0x00CF_9B00_0000_FFFF  ; Kernel code
    dq 0x00CF_9300_0000_FFFF  ; Kernel data
    dq 0x00CF_FB00_0000_FFFF  ; User code
    dq 0x00CF_F300_0000_FFFF  ; User data
    dq 0x0000_0000_0000_0000  ; Task state segment

; GDT Descriptor
gdt_desc:
    dw gdt_desc - gdt - 1
    dd gdt

section .bss
alignb 4

; Task State Segment
tss:
    resd 1    ; Not needed
global tss_esp0
tss_esp0:
    resd 1    ; ESP on switch to ring 0, set when task switching
tss_ss0:
    resd 1    ; SS on switch to ring 0
    resb TSS_SIZE - 12

; CPUID data
global cpuid_vendor
cpuid_vendor:     resb 13
global cpuid_brand
cpuid_brand:      resb 49
global cpuid_base_freq
cpuid_base_freq:  resw 1
