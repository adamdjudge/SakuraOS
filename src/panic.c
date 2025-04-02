#include <kernel.h>
#include <exception.h>
#include <x86.h>
#include <console.h>
#include <sched.h>

extern bool g_sched_active;

void panic(const char *msg)
{
    DISABLE_INTERRUPTS;
    g_sched_active = 0;

    printk("Kernel panic: %s\n", msg);
    for (;;) {}
}
