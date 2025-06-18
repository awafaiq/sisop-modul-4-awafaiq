prepare:
	mkdir -p bin

bootloader:
	nasm -f bin src/bootloader.asm -o bin/bootloader

stdlib:
	bcc -ansi -c -Iinclude src/std_lib.c -o bin/std_lib.o

kernel:
	bcc -ansi -c -Iinclude src/kernel.c -o bin/kernel.o
	nasm -f as86 src/kernel.asm -o bin/kernel_asm.o

link:
	ld86 -o bin/kernel-load -d bin/kernel.o bin/std_lib.o bin/kernel_asm.o
	dd if=bin/bootloader of=bin/floppy.img bs=512 count=1 conv=notrunc
	dd if=bin/kernel-load of=bin/floppy.img bs=512 seek=1 conv=notrunc

build: prepare bootloader stdlib kernel link

run:
	bochs -f bochsrc.txt
