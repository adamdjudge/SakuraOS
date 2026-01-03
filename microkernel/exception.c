/**
 * The SakuraOS Kernel
 * Copyright 2026 Adam Judge
 */

#include "kernel.h"
#include "exception.h"

void handle_exception(struct exception e)
{
    if (e.eno == ENO_IRQ0)
        debug("got timer interrupt!");
}
