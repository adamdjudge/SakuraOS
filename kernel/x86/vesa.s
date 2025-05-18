; ==============================================================================
; VESA video setup
; Copyright 2025 Adam Judge
; ==============================================================================

extern early_panic

global SetupVideo
global g_VesaInfo
global g_ModeInfo

struc VesaInfo
    .signature:     resb 4
    .version:       resw 1
    .oem:           resd 1
    .capabilities:  resd 1
    .modes_off:     resw 1
    .modes_seg:     resw 1
    .memory:        resw 1
    .softrev:       resw 1
    .vendor_off:    resw 1
    .vendor_seg:    resw 1
    .model_off:     resw 1
    .model_seg:     resw 1
    .rev_off:       resw 1
    .rev_seg:       resw 1
endstruc

struc ModeInfo
    .attributes:    resw 1
    .windowa:       resb 1
    .windowb:       resb 1
    .granularity:   resw 1
    .windowsize:    resw 1
    .segmenta:      resw 1
    .segmentb:      resw 1
    .winfnptr:      resd 1
    .pitch:         resw 1
    .width:         resw 1
    .height:        resw 1
    .wchar:         resb 1
    .ychar:         resb 1
    .planes:        resb 1
    .bpp:           resb 1
    .banks:         resb 1
    .memmodel:      resb 1
    .banksize:      resb 1
    .imgpages:      resb 1
    .resvd:         resb 1
    .redmask:       resb 1
    .redpos:        resb 1
    .greenmask:     resb 1
    .greenpos:      resb 1
    .bluemask:      resb 1
    .bluepos:       resb 1
    .resvdmask:     resb 1
    .resvdpos:      resb 1
    .dcattrs:       resb 1
    .framebuffer:   resd 1
    .offscrnoff:    resd 1
    .offscrnsize:   resw 1
endstruc

section .text
bits 16

SetupVideo:
    pusha

    ; Get VESA information from BIOS, specifying VBE 2.0+ support because we
    ; want linear frame buffers.
    mov di, g_VesaInfo
    mov dword [g_VesaInfo], 'VBE2'
    mov ax, 0x4F00
    int 0x10

    ; Check that it worked and that the BIOS supports VBE 2.0+.
    mov si, VesaPanicMsg
    cmp ax, 0x004F
    jne early_panic
    cmp dword [g_VesaInfo], 'VESA'
    jne early_panic
    cmp word [g_VesaInfo + VesaInfo.version], 0x0200
    jl early_panic

    ; Now get info about each supported mode, until we either find the one we're
    ; looking for (640x480 at 15 bpp) or hit the end of the list.
    mov ax, [g_VesaInfo + VesaInfo.modes_seg]
    mov ds, ax
    mov si, [g_VesaInfo + VesaInfo.modes_off]
    mov di, g_ModeInfo
.loop:
    lodsw
    cmp ax, 0xFFFF
    je .notfound
    mov cx, ax
    mov ax, 0x4F01
    int 0x10

    test word [g_ModeInfo + ModeInfo.attributes], 0x0080 ; Check for LFB
    je .loop
    cmp byte [g_ModeInfo + ModeInfo.bpp], 15
    jne .loop
    cmp word [g_ModeInfo + ModeInfo.width], 640
    jne .loop
    cmp word [g_ModeInfo + ModeInfo.height], 480
    jne .loop

    ; We now have the number of the mode we want, so let's switch to it!
    mov bx, cx
    or bx, 0x4000 ; Set bit 14 to use LFB
    mov ax, 0x4F02
    int 0x10

    popa
    ret

.notfound:
    popa
    mov si, ModePanicMsg
    jmp early_panic

section .bss
alignb 4

g_VesaInfo: resb 512
g_ModeInfo: resb 512

section .rodata

VesaPanicMsg:
    db "VESA VBE2 not supported", 0
ModePanicMsg:
    db "640x480x15 video mode not supported", 0
