1. Install qemu-system-x86, vim, git, make, binutils, gcc, nasm
2. Write bootloader.asm
   * size has to be 512 bytes
   * compile with: nasm -f bin bootloader.asm -o bootloader
   * -f flag is to produce raw binary without any extras
3. Create image
   * dd -if=bootloader of=disk.img bs=512 seek=0 count=1
4. Start qemu
   * qemu-system-i386 -machine q35 -fda disk.img


