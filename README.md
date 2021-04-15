# What is this?

This is an educational project in writing a x86, protected mode kernel in C.

1. The bootloader loads the kernel into memory, sets up initial segments, switches 
   to protected mode an jumps into the C kernel code.
2. The kernel sets up segments, interrupts, the programmable interrupt controller (PIC), paging, multi tasking and then jumps into user mode.
3. On timer interrupts, the kernel switches user taks in a round robin fashion.

# Build & Run
1. Install qemu-system-x86, vim, git, make, binutils, gcc, nasm
2. make run
