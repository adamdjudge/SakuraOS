CC = x86_64-linux-gnu-gcc
CFLAGS = -c -m32 -ffreestanding -fno-pie -std=c99 -Werror -Wall
AS = nasm
LD = x86_64-linux-gnu-ld

OBJ = start.o idt.o main.o io.o debug.o exception.o

kernel: link.x $(OBJ)
	$(LD) -T $< -o kernel $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

%.o: %.s
	$(AS) -f elf32 -o $@ $<

clean:
	rm -f kernel *.o
