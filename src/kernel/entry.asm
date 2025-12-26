bits 16

section _ENTRY class=CODE

extern _cstart_
global entry

entry:
    cli
    ; setup stack
    mov ax, ds
    mov ss, ax
    mov sp, 0
    mov bp, sp
    sti

    ; Call C main function
    call _cstart_

    cli
    hlt
