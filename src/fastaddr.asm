BITS 16
SECTION .text
GLOBAL _dos_shift_address

; unsigned dos_shift_address(unsigned base, unsigned long address,
;                            unsigned data, unsigned control)
; Shift all 24 address bits. Preserve data bits D2-D7 and every control bit
; except MUX, which is enabled. Return the last DATA value; leave MUX enabled
; so the C caller can apply its existing address-settle delay before routing
; back to the socket. No VCC/VPP/CE change, no interrupt masking, 8086 only.
_dos_shift_address:
    push bp
    mov bp, sp
    push bx
    push cx
    push dx
    push si
    push di
    mov dx, [bp+4]
    add dx, 2
    mov ax, [bp+12]
    or al, 2
    xor al, 0bh
    out dx, al
    sub dx, 2
    mov di, [bp+10]
    and di, 0fch
    mov ax, di
    out dx, al
    mov bx, [bp+6]
    mov si, [bp+8]
    mov cx, 24
.shift:
    mov ax, di
    test si, 0080h
    jz .zero
    or al, 2
.zero:
    out dx, al
    or al, 1
    out dx, al
    and al, 0feh
    out dx, al
    shl bx, 1
    rcl si, 1
    loop .shift
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop bp
    ret
