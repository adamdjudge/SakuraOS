; ==============================================================================
; The SakuraOS Kernel
; Copyright 2025 Adam Judge
; ==============================================================================

extern _kernel_bss
extern _kernel_end
extern detect_memory
extern setup_idt
extern kmain

%define KERNEL_CS 0x08
%define KERNEL_DS 0x10
%define KERNEL_TS 0x28

section .rodata

panic_str:
    db "Kernel panic: ", 0
bss_too_big:
    db "BSS too large for 640 kb low mem", 0
no_a20:
    db "can't enable A20", 0

; ==============================================================================
; Entry point from bootloader
; ==============================================================================

section .text

; Boot signature checked by bootloader.
boot_signature:
    dd 0xAA000515

bits 16

; 16-bit real mode entry point.
global kernel_entry
kernel_entry:
    cli
    mov esp, init_stack + 1024

    ; Sanity check that the BSS is not large enough to exceed the 640 kb of low
    ; memory and encroach on the BIOS area.
    mov si, bss_too_big
    mov eax, _kernel_end
    cmp eax, 0xA0000
    jge early_panic

    ; First clear out the BSS section. It's a little more complicated to do here
    ; in real mode due to segmentation, but it's best to do it now so that the
    ; rest of the real mode code can use the BSS safely.
    mov di, _kernel_bss
    mov ecx, _kernel_end
    sub ecx, _kernel_bss
    shr ecx, 2
    xor eax, eax
.loop:
    stosd
    cmp di, 0
    jne .continue
    mov dx, es
    add dx, 0x1000
    mov es, dx
.continue:
    loop .loop
    xor dx, dx
    mov es, dx

    ; Do the low-level setup that needs BIOS calls while still in real mode.
    call detect_memory

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
    jmp KERNEL_CS:enter_protected_mode

kbc_wait:
    mov cx, 10000
    loop $
    in al, 0x64
    test al, 2
    jnz kbc_wait
    ret

; Real mode print routine. SI = string pointer.
global early_printk
early_printk:
    mov ah, 0xE
.loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .loop
.done:
    ret

; Real mode kernel panic. SI = string pointer.
global early_panic
early_panic:
    push si
    mov si, panic_str
    call early_printk
    pop si
    call early_printk
.die:
    hlt
    jmp .die

bits 32

; Start of 32-bit protected mode code.
enter_protected_mode:
    mov ax, KERNEL_DS
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Set TSS segment base pointer manually.
    mov eax, tss
    or [gdt + KERNEL_TS + 2], eax

    ; Set TSS.SS0 manually since it's in BSS, and load the TSS.
    mov word [tss_ss0], KERNEL_DS
    mov ax, KERNEL_TS
    ltr ax

    ; Get CPUID processor info.
    xor eax, eax
    cpuid
    mov [g_cpuid_vendor], ebx
    mov [g_cpuid_vendor+4], edx
    mov [g_cpuid_vendor+8], ecx

    mov eax, 0x80000002
    cpuid
    mov [g_cpuid_brand], eax
    mov [g_cpuid_brand+4], ebx
    mov [g_cpuid_brand+8], ecx
    mov [g_cpuid_brand+12], edx

    mov eax, 0x80000003
    cpuid
    mov [g_cpuid_brand+16], eax
    mov [g_cpuid_brand+20], ebx
    mov [g_cpuid_brand+24], ecx
    mov [g_cpuid_brand+28], edx

    mov eax, 0x80000004
    cpuid
    mov [g_cpuid_brand+32], eax
    mov [g_cpuid_brand+36], ebx
    mov [g_cpuid_brand+40], ecx
    mov [g_cpuid_brand+44], edx

    mov eax, 0x16
    cpuid
    mov [g_cpuid_base_freq], ax

    ; Finish setting up low-level things, then call the main C function.
    call setup_idt
    call kmain

    ; After initialization, this thread becomes the idle thread.
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
    dq 0x0000_8900_0000_0068  ; Task state segment

; GDT Descriptor
gdt_desc:
    dw gdt_desc - gdt - 1
    dd gdt

section .bss
alignb 4

; Initial kernel stack used during startup
init_stack: resb 1024

; Task State Segment
tss:
    resd 1    ; Not needed
global tss_esp0
tss_esp0:
    resd 1    ; ESP on switch to ring 0, set when task switching
tss_ss0:
    resd 1    ; SS on switch to ring 0
    resb 192  ; Other fields we don't care about

; CPUID data
global g_cpuid_vendor
g_cpuid_vendor:     resb 13
global g_cpuid_brand
g_cpuid_brand:      resb 49
global g_cpuid_base_freq
g_cpuid_base_freq:  resw 1
