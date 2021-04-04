; bootloader, size is 512 byte
org 0x7c00 ; where to put the next piece of code related to the current segment
use16

; initialize segment selectors
xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax

; load sector two
mov ax, 0x0
mov es, ax     ; es:bx pointer to buffer
mov bx, 0x8000 ; es:bx pointer to buffer
mov ah, 0x02 ; function
mov al, 3 ; number of sectors to read
mov ch, 0 ; track
mov cl, 2 ; sector
mov dh, 0 ; head
mov dl, 0 ; drive
int 0x13
jc error ; c flag is set, if error during disk load occured

; turn off maskable interrupts
cli

; load descriptor table registers
lgdt [gdtr]
lidt [idtr]

; enable protected mode
mov eax, cr0
or eax, 0x1
mov cr0, eax

; far jump, sets code segments selector cs to 0x8 (first non-null descriptors)
jmp 0x8:protected_mode

; error message
error:
  mov si, .msg
.loop:
  lodsb
  or al, al
  jz .done
  mov ah, 0x0e
  int 0x10
  jmp .loop
.done:
  jmp $
  .msg db "could not read disk", 0

protected_mode:
  ; switch to 32 bit instructions
  use32
  ; initialize segment selectors
  mov ax, gdt.data  ; byte offset for selector 2
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax
  mov esp, 0x7c00 ; stack from 0x0000 to 0x7c00
  ; call loaded code
  call 0x8000
  jmp $

; global descriptor table
gdt:
  dq 0
  ; code
  .code equ $ - gdt
  dw 0xffff ; limit
  dw 0x0    ; base
  db 0x0    ; base
  db 0x9a   ; P (set, segment present, 0b1) / DPL (0, highest privilege, 0b00) / S (set, code or data segment, 0b1) / TYPE (code, executable, readable, 0b1010 = 0xa)
  db 0xcf   ; G (cleared, 4096 byte granularity of segment limit, 0b1), D/B (set, 32 bit segment, 0b1), L (cleared, 0b0), AVL (cleared, 0b0), limit (0xf)
  db 0x0    ; base
  ; data
  .data equ $ - gdt
  dw 0xffff ; limit
  dw 0x0    ; base
  db 0x0    ; base
  db 0x92   ; P (set, segment present, 0b1) / DPL (0, highest privilege, 0b00) / S (set, code or data segment, 0b1) / TYPE (data, read/write, 0b0010 = 0x2)
  db 0xcf   ; G (cleared, 4096 byte granularity of segment limit, 0b1), D/B (set, 32 bit segment, 0b1), L (cleared, 0b0), AVL (cleared, 0b0), limit (0xf)
  db 0x0    ; base
gdt_end:

; value for the global descriptor table register
gdtr: 
  dw gdt_end - gdt - 1 ; always one less than the real size
  dd gdt

; value for the interrupt descriptor table register
idtr:
  dw 0
  dd 0
  
times 510 - ($ - $$) db 0 ; clear remaining bytes with zeros; $ current address, $$ address of the current section, hence $ - $$ is the size of the above code
dw 0xAA55 ; last two bytes are the boot signature
