; ==============================================================================
; The SakuraOS Kernel
; Copyright 2025 Adam Judge
; ==============================================================================

extern schedule
extern init_pdir
extern thread
extern next_thread
extern proc
extern tss_esp0

; void inc_byte(uint8_t *val)
; Atomically increment a byte in memory.
global inc_byte
inc_byte:
    mov eax, [esp+4]
    lock inc byte [eax]
    ret

; void inc_word(uint16_t *val)
; Atomically increment a word in memory.
global inc_word
inc_word:
    mov eax, [esp+4]
    lock inc word [eax]
    ret

; void inc_dword(uint32_t *val)
; Atomically increment a dword in memory.
global inc_dword
inc_dword:
    mov eax, [esp+4]
    lock inc dword [eax]
    ret

; void dec_byte(uint8_t *val)
; Atomically decrement a byte in memory.
global dec_byte
dec_byte:
    mov eax, [esp+4]
    lock dec byte [eax]
    ret

; void dec_word(uint16_t *val)
; Atomically decrement a word in memory.
global dec_word
dec_word:
    mov eax, [esp+4]
    lock dec word [eax]
    ret

; void dec_dword(uint32_t *val)
; Atomically decrement a dword in memory.
global dec_dword
dec_dword:
    mov eax, [esp+4]
    lock dec dword [eax]
    ret

iowait:
    mov ecx, 0xffff
.loop:
    loop .loop
    ret

; void out_byte(unsigned short port, unsigned char data)
; Output byte to I/O port.
global out_byte
out_byte:
    mov edx, [esp+4]
    mov eax, [esp+8]
    out dx, al
    ret

; void out_byte_wait(unsigned short port, unsigned char data)
; Output byte to I/O port and then wait a short delay.
global out_byte_wait
out_byte_wait:
    mov edx, [esp+4]
    mov eax, [esp+8]
    out dx, al
    call iowait
    ret

; unsigned char in_byte(unsigned short port)
; Read a byte from I/O port.
global in_byte
in_byte:
    mov edx, [esp+4]
    in al, dx
    ret

; unsigned char in_byte(unsigned short port)
; Read a byte from I/O port.
global in_byte_wait
in_byte_wait:
    mov edx, [esp+4]
    in al, dx
    call iowait
    ret

; void memset(void *s, char c, unsigned int n)
; Efficiently fill a memory range with a byte value.
global memset
memset:
    push edi
    mov edi, [esp+8]
    mov eax, [esp+12]
    mov ecx, [esp+16]
    rep stosb
    pop edi
    ret

; void memcpy(void *dest, void *src, unsigned int n)
; Efficiently copy n bytes from src to dest.
global memcpy
memcpy:
    push esi
    push edi
    mov edi, [esp+12]
    mov esi, [esp+16]
    mov ecx, [esp+20]
    rep movsb
    pop edi
    pop esi
    ret

; void mutex_lock(mutex_t *mutex)
; Spinlock on a mutex. If the mutex is already locked, invoke the scheduler
; on each loop to give other threads the chance to release it.
global mutex_lock
mutex_lock:
    mov eax, [esp+4]
    mov cx, 1
.loop:
    xchg cl, [eax]
    jcxz .done
    push eax
    push ecx
    cli
    call schedule
    sti
    pop ecx
    pop eax
    jmp .loop
.done:
    ret

; void enable_paging()
global enable_paging
enable_paging:
    mov eax, init_pdir
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80010000
    mov cr0, eax
    ret

; void flush_tlb()
global flush_tlb
flush_tlb:
    mov eax, cr3
    mov cr3, eax
    ret

; void switch_context()
global switch_context
switch_context:
    pusha
    mov edi, [thread]
    mov esi, [next_thread]

    ; Stash current ESP and load next thread's ESP.
    mov [edi], esp
    mov esp, [esi]

    ; Set TSS.ESP0 to next thread's kernel stack.
    mov eax, [esi+4]
    mov [tss_esp0], eax

    ; Set the new current process.
    mov eax, [esi+8]
    mov [proc], eax

    ; If the new current process is not null and is different from the one being
    ; switched from, load its page directory.
    cmp eax, 0
    je .no_cr3
    cmp eax, [edi+8]
    je .no_cr3
    mov eax, [eax+4]
    mov cr3, eax

    ; Make next the new current and return to it.
.no_cr3:
    mov [thread], esi
    popa
    ret

; int execve(char *filename, char **argv, char **envp)
; Special execve implementation than can be called from kernel space.
global execve
execve:
    ; Make space for the esp and ss fields of the struct exception, since they
    ; aren't pushed by the CPU on ring 0 interrupt.
    push 0
    push 0

    mov eax, 11 ; temporary execve value
    mov ebx, [esp+12]
    mov ecx, [esp+16]
    mov edx, [esp+20]
    int 255

    add esp, 8
    ret
