; ==============================================================================
; The SakuraOS Bootloader
; Copyright 2025 Adam Judge
; ==============================================================================

extern __bss_start
extern __bss_end
extern __loram_top
extern main
extern memdetect

%define GDT_CS 0x08
%define GDT_DS 0x10

; ==============================================================================
; Real-mode entry point from boot sector
; ==============================================================================

section .text.start

; Boot signature checked by bootloader.
boot_signature:
    dd 0xAA000515

bits 16

; 16-bit real mode entry point.
global _start
_start:
    cli
    call memdetect

    ; Try to enable A20 through the keyboard controller.
    call kbc_wait
    mov al, 0xD1
    out 0x64, al
    call kbc_wait
    mov al, 0xDF
    out 0x60, al
    call kbc_wait

    ; Load GDT and enter protected mode.
    lgdt [gdt_desc]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp GDT_CS:enter_protected_mode

kbc_wait:
    mov cx, 10000
    loop $
    in al, 0x64
    test al, 2
    jnz kbc_wait
    ret

bits 32

; Start of 32-bit protected mode code.
enter_protected_mode:
    mov ax, GDT_DS
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov esp, __loram_top
    call main
.halt:
    hlt
    jmp .halt

; ==============================================================================
; Processor Data Structures
; ==============================================================================

align 2

; GDT Descriptor
; Kept in .text.start to ensure it doesn't go out of range of 16-bit address
gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

section .rodata
align 8

; Global Descriptor Table
gdt_start:
    dq 0x0000_0000_0000_0000  ; Null segment
    dq 0x00CF_9B00_0000_FFFF  ; Kernel code
    dq 0x00CF_9300_0000_FFFF  ; Kernel data
    dq 0x00CF_FB00_0000_FFFF  ; User code
    dq 0x00CF_F300_0000_FFFF  ; User data
    dq 0x0000_8900_0000_0068  ; Task state segment
gdt_end:
