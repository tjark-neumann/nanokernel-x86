# nanokernel build
CC      := gcc
LD      := ld
NASM    := nasm

CFLAGS  := -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib \
           -Wall -Wextra -O2 -std=gnu11
LDFLAGS := -m elf_i386 -T linker.ld -nostdlib

CSRC := $(wildcard kernel/*.c)
COBJ := $(CSRC:.c=.o)

all: os.img

boot.bin: boot.asm
	$(NASM) -f bin $< -o $@

kernel/entry.o: kernel/entry.asm
	$(NASM) -f elf32 $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.elf: kernel/entry.o $(COBJ)
	$(LD) $(LDFLAGS) -o $@ $^

kernel.bin: kernel.elf
	objcopy -O binary $< $@

os.img: boot.bin kernel.bin
	cat boot.bin kernel.bin > os.img
	truncate -s 65536 os.img

run: os.img
	qemu-system-i386 -drive format=raw,file=os.img -serial stdio -display none

run-gui: os.img
	qemu-system-i386 -drive format=raw,file=os.img

clean:
	rm -f boot.bin kernel.bin kernel.elf os.img kernel/*.o

.PHONY: all run run-gui clean
