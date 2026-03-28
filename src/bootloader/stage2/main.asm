bits 16

section .text

extern cstart_
global entry

entry:
    cli
    ; setup stack
    mov ax, ds
    mov ss, ax
    mov esp, 0x0000FFF0
    sti

    ; expect boot drive in dl, send it as argument to cstart function
    xor dh, dh
    movzx edx, dx
    push dword edx
    call dword cstart_
    add esp, 4

    cli
    hlt