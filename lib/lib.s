extern main

%macro syscall0 2
global %2
%2:
    mov eax, %1
    jmp _do_syscall
%endmacro

%macro syscall1 2
global %2
%2:
    mov eax, %1
    mov ebx, [esp+4]
    jmp _do_syscall
%endmacro

%macro syscall2 2
global %2
%2:
    mov eax, %1
    mov ebx, [esp+4]
    mov ecx, [esp+8]
    jmp _do_syscall
%endmacro

%macro syscall3 2
global %2
%2:
    mov eax, %1
    mov ebx, [esp+4]
    mov ecx, [esp+8]
    mov edx, [esp+12]
    jmp _do_syscall
%endmacro

section .text

global start
start:
    xor eax, eax
    mov [errno], eax
    call main
    mov ebx, eax
    xor eax, eax
    int 255
    jmp $

_do_syscall:
    int 255
    cmp eax, 0
    jge .noerror
    neg eax
    mov [errno], eax
    mov eax, -1
.noerror:
    ret

syscall0 -1, _sigreturn
syscall1 0, _exit
syscall3 1, waitpid
syscall1 2, alarm
syscall2 3, kill
syscall2 4, signal
syscall0 12, fork
syscall3 11, execve
syscall1 69, print

section .bss
alignb 4
errno:
    resb 4
