# Makefile - Linux, WSL, MSYS2 Bash에서 사용
# 필요 도구: nasm, gcc, ld, objcopy, dd, qemu-system-i386

KERNEL_SECTORS = 32
IMAGE_SECTORS  = 33

all: os-image.bin

boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin

kernel_entry.o: kernel_entry.asm
	nasm -f elf32 kernel_entry.asm -o kernel_entry.o

kernel.o: kernel.c
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -O2 -Wall -Wextra -c kernel.c -o kernel.o

kernel.elf: kernel_entry.o kernel.o linker.ld
	ld -m elf_i386 -T linker.ld -o kernel.elf kernel_entry.o kernel.o

kernel.bin: kernel.elf
	objcopy -O binary kernel.elf kernel.bin

os-image.bin: boot.bin kernel.bin
	dd if=/dev/zero of=os-image.bin bs=512 count=$(IMAGE_SECTORS)
	dd if=boot.bin of=os-image.bin conv=notrunc
	dd if=kernel.bin of=os-image.bin bs=512 seek=1 conv=notrunc

run: os-image.bin
	qemu-system-i386 -drive format=raw,file=os-image.bin

run-floppy: os-image.bin
	qemu-system-i386 -fda os-image.bin

run-debug: os-image.bin
	qemu-system-i386 -drive format=raw,file=os-image.bin -no-reboot -no-shutdown -d int,cpu_reset

clean:
	rm -f *.bin *.o *.elf os-image.bin
