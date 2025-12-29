CC = x86_64-linux-gnu-gcc
CFLAGS = -c -m32 -ffreestanding -fno-pie -std=c99 -Werror -Wall
AS = nasm
LD = x86_64-linux-gnu-ld
OC = x86_64-linux-gnu-objcopy

OBJ = start.o memdetect.o main.o console.o string.o

disk: bootsect.s template.img stage2
	$(AS) -f bin -o boot.img $<
	tail -c +513 template.img >> boot.img
	sudo mount boot.img /mnt && sudo cp stage2 /mnt && sudo umount /mnt

stage2: link.x $(OBJ)
	$(LD) -T $< -o stage2.elf $(OBJ)
	$(OC) -O binary stage2.elf stage2

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

%.o: %.s
	$(AS) -f elf32 -o $@ $<

run: disk
	qemu-system-i386 -fda boot.img

clean:
	rm -f boot.img stage2 stage2.elf *.o
