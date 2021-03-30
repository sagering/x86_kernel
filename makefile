bootloader: bootloader.asm
	nasm -f bin $< -o $@

os: os.asm
	nasm -f bin $< -o $@

bootdisk: bootloader os
	dd if=/dev/zero of=bootdisk.img bs=512 count=2
	dd conv=notrunc if=bootloader of=bootdisk.img bs=512 seek=0 count=1
	dd conv=notrunc if=os of=bootdisk.img bs=512 seek=1 count=1

run: bootdisk
	qemu-system-i386 -machine q35 -fda bootdisk.img
