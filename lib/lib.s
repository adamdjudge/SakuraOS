; ==============================================================================
; The SakuraOS Standard Library
; Copyright 2025 Adam Judge
; File: lib.s
; Description: Core standard library functions for startup and system calls.
; ==============================================================================

extern main

section .bss

; The errno global variable, which indicates system call errors
; TODO: This needs to be made per-thread rather than just a global variable
global errno
alignb 4
errno:
    resb 4

; ==============================================================================
; Program entry point
; ==============================================================================

section .text

global start
start:
    call main
    mov ebx, eax
    xor eax, eax
    int 255
    jmp $

; ==============================================================================
; System calls
; ==============================================================================

; 0-argument system call template
%macro syscall0 2
global %2
%2:
    mov eax, %1
    int 255
    jmp _check_error
%endmacro

; 1-argument system call template
%macro syscall1 2
global %2
%2:
    push ebx
    mov eax, %1
    mov ebx, [esp+8]
    int 255
    pop ebx
    jmp _check_error
%endmacro

; 2-argument system call template
%macro syscall2 2
global %2
%2:
    push ebx
    mov eax, %1
    mov ebx, [esp+8]
    mov ecx, [esp+12]
    int 255
    pop ebx
    jmp _check_error
%endmacro

; 3-argument system call template
%macro syscall3 2
global %2
%2:
    push ebx
    mov eax, %1
    mov ebx, [esp+8]
    mov ecx, [esp+12]
    mov edx, [esp+16]
    int 255
    pop ebx
    jmp _check_error
%endmacro

; 4-argument system call template
%macro syscall4 2
global %2
%2:
    push ebx
    push esi
    mov eax, %1
    mov ebx, [esp+12]
    mov ecx, [esp+16]
    mov edx, [esp+20]
    mov esi, [esp+24]
    int 255
    pop esi
    pop ebx
    jmp _check_error
%endmacro

; 5-argument system call template
%macro syscall5 2
global %2
%2:
    push ebx
    push esi
    push edi
    mov eax, %1
    mov ebx, [esp+16]
    mov ecx, [esp+20]
    mov edx, [esp+24]
    mov esi, [esp+28]
    mov edi, [esp+32]
    int 255
    pop edi
    pop esi
    pop ebx
    jmp _check_error
%endmacro

; Update errno if needed after system call
_check_error:
    cmp eax, 0
    jge .noerror
    neg eax
    mov [errno], eax
    mov eax, -1
.noerror:
    ret

; System call table
syscall1 0, _exit
syscall0 1, fork
syscall3 2, execve
syscall3 3, waitpid
syscall2 4, signal
syscall1 5, alarm
syscall2 6, kill
syscall3 7, open
syscall1 8, close
syscall3 9, read
syscall3 10, write
