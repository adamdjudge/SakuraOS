CC = gcc -c -m32 -ffreestanding -fno-leading-underscore -fno-pie -Iinclude -std=c99 -Werror -Wall
AS = nasm -f elf32

OBJ = \
	x86/start.o \
	x86/memdetect.o \
	x86/idt.o \
	x86/asm.o \
	main.o \
	serial.o \
	console.o \
	ps2.o \
	exception.o \
	syscall.o \
	mm.o \
	printk.o \
	panic.o \
	sched.o \
	signal.o \
	init.o \
	floppy.o \
	buffer.o \
	blkdev.o \
	chrdev.o \
	tty.o \
	super.o \
	inode.o \
	file.o \
	exec.o

kernel: link.ld $(OBJ)
	ld -T $< -o kernel.elf $(OBJ)
	objcopy -O binary kernel.elf kernel
	objdump -M intel -D kernel.elf > disasm.txt

%.o: %.c
	$(CC) -o $@ $<

%.o: %.s
	$(AS) -o $@ $<

clean:
	rm -f kernel kernel.elf disasm.txt $(OBJ)
