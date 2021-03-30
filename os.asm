org 0x8000
use32

mov ebx, 0xb8000 ; The video address
mov al, 'H'      ; The character to be print
mov ah, 0x0F     ; The color: white(F) on black(0)
mov [ebx], ax

mov ebx, 0xb8002 ; The video address
mov al, 'i'      ; The character to be print
mov ah, 0x0F     ; The color: white(F) on black(0)
mov [ebx], ax

ret
