CC = gcc -c -m32 -ffreestanding -fno-pie -std=c99 -Wall -Werror
AS = nasm -f elf32

OBJ = lib.o init.o

all: init

%.o: %.c
	$(CC) -o $@ $<

%.o: %.s
	$(AS) -o $@ $<

init: link.ld $(OBJ)
	ld -T $< -o init $(OBJ)

clean:
	rm -f init $(OBJ)
