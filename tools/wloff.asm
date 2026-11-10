; 8086 DOS .COM: safe idle for this PCB3B Willem at LPT1 378h.
; Same logical state as wl_safe(): DATA=00, CONTROL=0B (XOR 0B).
; First deassert VPP alone, preserving the remaining control bits.
bits 16
cpu 8086
org 100h
    mov dx,37ah
    in al,dx
    or al,1
    out dx,al
    mov al,0bh
    out dx,al
    mov dx,378h
    xor al,al
    out dx,al
    mov dx,message
    mov ah,9
    int 21h
    mov ax,4c00h
    int 21h
message db 'Willem LPT378: VPP OFF, VCC OFF; DATA=00 CONTROL=0B written.',13,10,'$'
