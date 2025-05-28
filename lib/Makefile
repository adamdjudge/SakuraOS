CC = gcc -c -m32 -ffreestanding -fno-pie -std=c99 -Wall -Werror
AS = nasm -f elf32

OBJ = lib.o

libsakura.a: $(OBJ)
	ar rcs $@ $^

%.o: %.c
	$(CC) -o $@ $<

%.o: %.s
	$(AS) -o $@ $<

clean:
	rm -f *.o *.a
