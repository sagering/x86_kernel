bootloader.bin: bootloader.asm
	nasm -f bin $< -o $@

main.bin: main.c linker.lds
	gcc -c -m32 -nostdlib -nodefaultlibs -fno-exceptions -static main.c -fno-pie -fno-builtin -mgeneral-regs-only
	ld -melf_i386 -o main.bin -T linker.lds main.o

bootdisk.img: bootloader.bin main.bin
	dd if=/dev/zero of=bootdisk.img bs=512 count=4
	dd conv=notrunc if=bootloader.bin of=bootdisk.img bs=512 seek=0 count=1
	dd conv=notrunc if=main.bin of=bootdisk.img bs=512 seek=1 count=3

run: bootdisk.img
	qemu-system-i386 -machine q35 -fda bootdisk.img

.PHONY: clean

clean:
	rm *.bin *.o *.img
