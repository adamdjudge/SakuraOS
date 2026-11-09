CC = x86_64-linux-gnu-gcc
AS = nasm
LD = x86_64-linux-gnu-ld

CFLAGS = -c -m32 -ffreestanding -fno-pie -std=c99 -Werror -Wall
ASFLAGS = -f elf32

ARCH_OBJS = entry.o \
            start.o \
		    io.o \
		    uart.o

ARCH_LINKER_SCRIPT = link.lds
