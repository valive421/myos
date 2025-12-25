
times 510-($-$$) db 0
dw 0xAA55
bits 16
org 0

%define Endl 0x0D, 0x0A

start:
    jmp main

; puts string to screen
puts:
    push si
    push ax

.loop:
    lodsb
    cmp al, 0
    je .done
    mov ah, 0x0E
    int 0x10
    jmp .loop

.done:
    pop ax
    pop si
    ret

main:
    ; Set up segment registers for 0x2000:0
    mov ax, 0x2000
    mov ds, ax
    mov es, ax

    ; Set up stack (below kernel)
    mov ss, ax
    mov sp, 0x7c00

    ; Print hello message
    mov si, msg_hello
    call puts
    cli
    hlt

.halt:
    jmp .halt

msg_hello: db 'Hello, World! from kernel!q', Endl, 0
