; bootloader, size is 512 byte
org 0x7c00 ; where to put the next piece of code related to the current segment
bits 16
start: jmp boot

boot:
  cli ; ignore maskable interrupts
  cld ; set DF flag to zero => esi and edi are incremented instead of decremented for string opertaions 

load_sector_2:
  mov ax, 0x50
  mov es, ax ; es:bx pointer to buffer
  xor bx, bx ; es:bx pointer to buffer

  mov ah, 0x02 ; function
  mov al, 1 ; number of sectors to read
  mov ch, 0 ; track
  mov cl, 2 ; sector
  mov dh, 0 ; head
  mov dl, 0 ; drive

  int 0x13
  jmp 0x50:0

done:
   hlt

times 510 - ($ - $$) db 0 ; clear remaining bytes with zeros; $ current address, $$ address of the current section, hence $ - $$ is the size of the above code
dw 0xAA55 ; last two bytes are the boot signature
