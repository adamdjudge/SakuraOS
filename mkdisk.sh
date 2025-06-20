#!/bin/bash

# SakuraOS boot disk creation utility
# Copyright 2025 Adam Judge

cd kernel
make || exit 1
cd ../lib
make || exit 1
cd ../bin
./mkbins.sh || exit 1
cd ../boot
nasm -o bootsect.bin -f bin boot.s || exit 1
cd ..

cp boot/bootsect.bin sakura.img
tail -c +513 boot/template.img >> sakura.img
sudo mount sakura.img /mnt

sudo cp kernel/kernel /mnt
sudo mkdir /mnt/dev
sudo mknod /mnt/dev/fd0 b 2 0
sudo mknod /mnt/dev/tty0 c 4 0
sudo mknod /mnt/dev/tty1 c 4 1
sudo cp -r bin /mnt
sudo cp test.txt /mnt

sudo umount /mnt

[ "$1" == "run" ] && qemu-system-i386 -fda sakura.img -serial stdio -m 16
