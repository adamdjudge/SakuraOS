CC = gcc -c -m32 -ffreestanding -fno-leading-underscore -fno-pie -Iinclude -std=c99 -Werror -Wall
AS = nasm -f elf32

OBJ = \
	src/x86/start.o \
	src/x86/memdetect.o \
	src/x86/idt.o \
	src/x86/asm.o \
	src/main.o \
	src/serial.o \
	src/console.o \
	src/exception.o \
	src/syscall.o \
	src/mm.o \
	src/printk.o \
	src/panic.o \
	src/sched.o \
	src/signal.o \
	src/init.o \
	src/floppy.o \
	src/buffer.o \
	src/blkdev.o \
	src/super.o \
	src/inode.o \
	src/file.o \
	src/exec.o

all: kernel

%.o: %.c
	$(CC) -o $@ $<

%.o: %.s
	$(AS) -o $@ $<

kernel: link.ld $(OBJ)
	ld -T $< -o kernel.elf $(OBJ)
	objcopy -O binary kernel.elf kernel
	objdump -M intel -D kernel.elf > disasm.txt

bootsect: boot/boot.s
	nasm -o boot/bootsect.bin -f bin $<

clean:
	rm -f kernel kernel.elf disasm.txt sakura.img boot/bootsect.bin $(OBJ)
