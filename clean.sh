#!/bin/bash

cd kernel
make clean
cd ../init
make clean
cd ../boot
rm bootsect.bin
cd ..
rm sakura.img

