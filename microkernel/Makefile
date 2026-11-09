CC = x86_64-linux-gnu-gcc
AS = nasm
LD = x86_64-linux-gnu-ld

CFLAGS = -c -m32 -ffreestanding -fno-pie -std=c99 -Werror -Wall
ASFLAGS = -f elf32

#OBJ = start.o idt.o main.o io.o debug.o exception.o mm.o sched.o
OBJ = start.o \
      main.o \
	  io.o \
	  debug.o \
	  uart.o

kernel: link.x $(OBJ)
	$(LD) -T $< -o kernel $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

run: kernel
	qemu-system-i386 -serial stdio -kernel kernel

clean:
	rm -f kernel *.o
