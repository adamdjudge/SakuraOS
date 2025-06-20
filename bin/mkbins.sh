#!/bin/bash

INCLUDE=../lib/include
LIB=../lib/libsakura.a

function build() {
    gcc -c -m32 -ffreestanding -fno-pie -std=c99 -Wall -Werror -I $INCLUDE -o $1.o $1.c || exit 1
    ld -T link.ld -o $1 $1.o $LIB
}

build init
