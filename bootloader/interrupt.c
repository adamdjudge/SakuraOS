/**
 * The SakuraOS Bootloader
 * Copyright 2026 Adam Judge
 */

#include "bootloader.h"

#define GATE_INT 0x8E
#define GATE_TRAP 0x8F

#define PIC0_CMD 0x20
#define PIC0_DATA 0x21
#define PIC1_CMD 0xA0
#define PIC1_DATA 0xA1
#define PIC_EOI 0x20

#define PIT_CMD 0x43
#define PIT_DATA 0x40
#define PIT_DIVIDER 1193

/* i386 exception numbers */
enum {
    DIVISION_BY_ZERO = 0,
    DEBUG = 1,
    NMI = 2,
    BREAKPOINT = 3,
    INTO_DETECTED_OVERFLOW = 4,
    OUT_OF_BOUNDS = 5,
    INVALID_OPCODE = 6,
    NO_COPROCESSOR = 7,
    DOUBLE_FAULT = 8,
    COPROCESSOR_SEGMENT_OVERRUN = 9,
    BAD_TSS = 10,
    SEGMENT_NOT_PRESENT = 11,
    STACK_FAULT = 12,
    GENERAL_PROTECTION_FAULT = 13,
    PAGE_FAULT = 14,
    RESERVED = 15,
    COPROCESSOR_FAULT = 16,
    ALIGNMENT_CHECK = 17,
    MACHINE_CHECK = 18,
    SIMD_FP = 19,
    IRQ0 = 32,
    IRQ1 = 33,
    IRQ2 = 34,
    IRQ3 = 35,
    IRQ4 = 36,
    IRQ5 = 37,
    IRQ6 = 38,
    IRQ7 = 39,
    IRQ8 = 40,
    IRQ9 = 41,
    IRQ10 = 42,
    IRQ11 = 43,
    IRQ12 = 44,
    IRQ13 = 45,
    IRQ14 = 46,
    IRQ15 = 47,
};

volatile static struct {
    uint16_t offset_lo;
    uint16_t segment;
    uint8_t resvd;
    uint8_t type;
    uint16_t offset_hi;
} __attribute__((aligned(8))) idt[256];

volatile static struct __attribute__((packed)) {
    uint16_t size;
    uint32_t offset;
} idtdesc;

static void set_gate(uint8_t id, uint8_t type, uint32_t ptr)
{
    idt[id].segment = 8;
    idt[id].type = type;
    idt[id].offset_lo = ptr & 0xffff;
    idt[id].offset_hi = ptr >> 16;
}

static volatile unsigned int timer;
extern void floppy_update_timer();

__attribute__((interrupt)) static void timer_irq(void *_frame)
{
    if (timer)
        timer--;
    floppy_update_timer();
    out_byte(PIC0_CMD, PIC_EOI);
}

extern void handle_floppy_irq();

__attribute__((interrupt)) static void floppy_irq(void *_frame)
{
    handle_floppy_irq();
    out_byte(PIC0_CMD, PIC_EOI);
}

void interrupt_init()
{
    /* Reprogram the PICs to map IRQs to vectors 32-47. */
    out_byte_wait(PIC0_CMD, 0x11);
    out_byte_wait(PIC0_DATA, 0x20);
    out_byte_wait(PIC0_DATA, 0x04);
    out_byte_wait(PIC0_DATA, 0x01);
    out_byte_wait(PIC0_DATA, 0x00);
    out_byte_wait(PIC1_CMD, 0x11);
    out_byte_wait(PIC1_DATA, 0x28);
    out_byte_wait(PIC1_DATA, 0x02);
    out_byte_wait(PIC1_DATA, 0x01);
    out_byte_wait(PIC1_DATA, 0x00);

    /* Set the timer to approx 1000 Hz. */
    out_byte_wait(PIT_CMD, 0x36);
    out_byte_wait(PIT_DATA, PIT_DIVIDER & 0xff);
    out_byte_wait(PIT_DATA, PIT_DIVIDER >> 8);

    for (int i = 0; i < 256; i++)
        idt[i].type = 0;

    set_gate(IRQ0, GATE_INT, (uint32_t)timer_irq); /* Timer interrupt */
    set_gate(IRQ6, GATE_INT, (uint32_t)floppy_irq); /* Floppy interrupt */

    idtdesc.size = sizeof(idt) - 1;
    idtdesc.offset = (uint32_t)&idt;
    __asm__ __volatile__("lidt idtdesc; sti" ::: "memory");
}

void delay(unsigned int ms)
{
    timer = ms;
    while (timer);
}
