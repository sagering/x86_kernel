org 0x0000
bits 16

mov ax, cs ; not sure
mov ds, ax ; not sure

print_msg:
  mov si, msg

print_msg_repeat:
  mov al, [si]
  add si, 1

  cmp al, 0
  je print_msg

  mov ah, 0xe
  mov bh, 0x0 ; page number
  mov bl, 0x0 ; foreground color
  int 0x10
  jmp print_msg_repeat

 msg db "Hello World!\n", 0
