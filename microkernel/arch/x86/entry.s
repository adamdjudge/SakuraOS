; ==============================================================================
; The SakuraOS Kernel
; Copyright 2026 Adam Judge
; ==============================================================================

extern __kernel_bss
extern __kernel_end
extern start

; ==============================================================================
; Multiboot header
; ==============================================================================

MULTIBOOT_PAGE_ALIGN   equ 0x1
MULTIBOOT_MEMORY_INFO  equ 0x2

MULTIBOOT_HEADER_MAGIC equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO

section .multiboot
align 4
    dd MULTIBOOT_HEADER_MAGIC
    dd MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
    dd -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

; ==============================================================================
; Entry point from bootloader.
; By now the bootloader has already enabled A20 and entered protected mode.
; ==============================================================================

section .text
bits 32

global _entry
_entry:
    mov esp, init_stack + 1024

    ; Zero out the BSS.
    mov edi, __kernel_bss
    mov ecx, __kernel_end
    sub ecx, __kernel_end
    shr ecx, 2
    add ecx, 1
    xor eax, eax
    rep stosd

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

    ; Start running C code.
    call start

.halt:
    hlt
    jmp .halt

; ==============================================================================
; Data Sections
; ==============================================================================

section .bss
alignb 4

; CPUID data
global cpuid_vendor
cpuid_vendor:     resb 13
global cpuid_brand
cpuid_brand:      resb 49
global cpuid_base_freq
cpuid_base_freq:  resw 1

alignb 4

; Initial kernel stack
init_stack: resb 1024
