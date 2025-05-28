CC = gcc -c -m32 -ffreestanding -fno-pie -std=c99 -Wall -Werror -I../lib/include
AS = nasm -f elf32

OBJ = init.o
LIB = ../lib/libsakura.a

init: link.ld $(OBJ) $(LIB)
	ld -T $< -o init $(OBJ) $(LIB)

%.o: %.c
	$(CC) -o $@ $<

%.o: %.s
	$(AS) -o $@ $<

clean:
	rm -f init $(OBJ)
