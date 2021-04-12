bootloader.bin: bootloader.asm
	nasm -f bin $< -o $@

main.bin: main.c linker.lds
	gcc -c -m32 -nostdlib -nodefaultlibs -fno-exceptions -static main.c -fno-pie -fno-builtin -mgeneral-regs-only
	ld -melf_i386 -o main.bin -T linker.lds main.o

main.inspect: main.c
	gcc -c -m32 -nostdlib -nodefaultlibs -fno-exceptions -static main.c -o main.inspect.o -fno-pie -fno-builtin -mgeneral-regs-only
	objdump -S main.inspect.o > main.inspect

playground: playground.c
	gcc -c -m32 -nostdlib -nodefaultlibs -fno-exceptions -static playground.c -fno-pie -fno-builtin -mgeneral-regs-only

bootdisk.img: bootloader.bin main.bin
	dd if=/dev/zero of=bootdisk.img bs=512 count=17
	dd conv=notrunc if=bootloader.bin of=bootdisk.img bs=512 seek=0 count=1
	dd conv=notrunc if=main.bin of=bootdisk.img bs=512 seek=1 count=16

run: bootdisk.img
	qemu-system-i386 -machine q35 -fda bootdisk.img -monitor stdio

.PHONY: clean main.inspect

clean:
	rm *.bin *.o *.img *.inspect
