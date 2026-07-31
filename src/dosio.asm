BITS 16

SECTION .text

GLOBAL _dos_outb
GLOBAL _dos_inb
GLOBAL _dos_wait_us
GLOBAL _dos_datetime

; void dos_outb(unsigned port, unsigned value)
_dos_outb:
    push bp
    mov bp, sp
    mov dx, [bp+4]
    mov ax, [bp+6]
    out dx, al
    pop bp
    ret

; unsigned dos_inb(unsigned port)
_dos_inb:
    push bp
    mov bp, sp
    mov dx, [bp+4]
    in al, dx
    xor ah, ah
    pop bp
    ret

; void dos_wait_us(unsigned usec)
; Non-destructively latch/read PIT channel 0.  Its 1.193182 MHz clock is
; independent of 8086/V30 instruction speed, so a faster V30 cannot shorten
; programmer setup or pulse timing.  The largest caller delay is 50 ms, less
; than one 16-bit PIT period (~54.9 ms), making modulo subtraction unambiguous.
_dos_wait_us:
    push bp
    mov bp, sp
    push bx
    push cx
    push dx
    mov ax, [bp+4]
    or ax, ax
    jz .done
    ; 1.25 ticks/us is deliberately rounded above the real 1.193182 value.
    ; 50,000 us becomes 62,500 ticks and still fits in 16 bits.
    mov bx, ax
    shr ax, 1
    shr ax, 1
    add bx, ax

    ; Latch and read the starting channel-0 down-counter value.
    xor al, al
    out 043h, al
    in al, 040h
    mov dl, al
    in al, 040h
    mov dh, al
    mov cx, dx
.pit_loop:
    xor al, al
    out 043h, al
    in al, 040h
    mov dl, al
    in al, 040h
    mov dh, al
    mov ax, cx
    sub ax, dx                  ; modulo arithmetic also handles PIT wrap
    cmp ax, bx
    jb .pit_loop
.done:
    pop dx
    pop cx
    pop bx
    pop bp
    ret

; void dos_datetime(unsigned fields[7])
; year, month, day, hour, minute, second, hundredth
_dos_datetime:
    push bp
    mov bp, sp
    push bx
    push si
    push di
    mov di, [bp+4]
    mov ah, 02ah
    int 021h
    mov [di], cx
    xor ax, ax
    mov al, dh
    mov [di+2], ax
    mov al, dl
    mov [di+4], ax
    mov ah, 02ch
    int 021h
    xor ax, ax
    mov al, ch
    mov [di+6], ax
    mov al, cl
    mov [di+8], ax
    mov al, dh
    mov [di+10], ax
    mov al, dl
    mov [di+12], ax
    pop di
    pop si
    pop bx
    pop bp
    ret
