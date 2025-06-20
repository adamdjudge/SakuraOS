#!/bin/bash

cd kernel
make clean
cd ../bin
./cleanbins.sh
cd ../lib
make clean
cd ../boot
rm bootsect.bin
cd ..
rm sakura.img

