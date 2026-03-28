
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
    ; Set up segment registers for current load segment
    mov ax, cs
    mov ds, ax
    mov es, ax

    ; Set up stack in current segment
    mov ss, ax
    mov sp, 0xFFFE

    ; Print hello 
    mov si, msg_hello
    call puts
    cli
    hlt

.halt:
    jmp .halt

msg_hello: db 'Hello from kernel!', Endl, 0
