bits 16

section _TEXT class=CODE

; Video output function
global _x86_Video_WriteCharTeletype
_x86_Video_WriteCharTeletype:
    push bp
    mov bp, sp
    push bx
    
    mov ah, 0Eh
    mov al, [bp + 4]
    mov bh, [bp + 6]
    int 10h
    
    pop bx
    mov sp, bp
    pop bp
    ret

; Port I/O functions
global _x86_inb
_x86_inb:
    push bp
    mov bp, sp
    
    mov dx, [bp + 4]
    xor ax, ax
    in al, dx
    
    mov sp, bp
    pop bp
    ret

global _x86_outb
_x86_outb:
    push bp
    mov bp, sp
    
    mov dx, [bp + 4]
    mov al, [bp + 6]
    out dx, al
    
    mov sp, bp
    pop bp
    ret
