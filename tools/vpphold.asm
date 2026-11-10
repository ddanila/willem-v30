; RF2/standard 2716 meter hold on the established PCB3B LPT378 setup.
; No programming pulse: CE/PGM stays low, OE high, data FF.
; BIOS time-of-day ticks avoid the unresolved raw PIT mode-3 delay bug.
; Fixed ~30 s hold; no DOS calls while either rail is enabled.
bits 16
cpu 8086
org 100h
    mov dx,37ah
    in al,dx
    or al,1                 ; VPP off before any other transition
    out dx,al
    mov al,0bh              ; both rails off, CE low, OE high
    out dx,al
    mov dx,378h
    mov al,0ffh             ; benign all-ones data
    out dx,al
    mov dx,37ah
    mov al,0fh              ; VCC on, VPP off, CE low, OE high
    out dx,al
    mov bx,2                ; >= one full BIOS tick of VCC settling
    call wait_ticks
    mov dx,37ah
    mov al,0eh              ; VPP on; CE remains low (no pulse)
    out dx,al
    mov bx,547              ; about 30 seconds
    call wait_ticks
    mov dx,37ah
    mov al,0fh              ; VPP off first
    out dx,al
    mov al,0bh              ; then VCC off
    out dx,al
    mov dx,378h
    xor al,al
    out dx,al
    mov dx,message
    mov ah,9
    int 21h
    mov ax,4c00h
    int 21h
wait_ticks:
    xor ah,ah
    int 1ah
    mov si,dx
.loop:
    xor ah,ah
    int 1ah
    sub dx,si
    cmp dx,bx
    jb .loop
    ret
message db 'VPP meter hold finished: VPP OFF, VCC OFF; no programming pulses.',13,10,'$'
