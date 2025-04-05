#include <kernel.h>
#include <exception.h>
#include <x86.h>
#include <console.h>
#include <sched.h>

void panic(const char *msg)
{
    DISABLE_INTERRUPTS;
    printk("Kernel panic: %s\n", msg);
    for (;;) {}
}
