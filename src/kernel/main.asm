org 0x7c00
bits 16


%define Endl 0x0D, 0x0A


start:
    jmp main



;puts string to screen
puts:
    push si
    push ax

.loop:
    lodsb ; load byte at DS:SI into AL and increment SI
    cmp al, 0 ; check for null terminator
    je .done  ; if null terminator, we're done
    mov ah, 0x0E  ; BIOS teletype function
    int 0x10  ; call BIOS interrupt
    jmp .loop  ; repeat for next character

.done:
    pop ax  ; restore ax
    pop si   ; restore si
    ret    ; return from function
main:
    ;set up segment registers
    mov ax, 0
    mov ds, ax
    mov es, ax

    ;set up stack
    mov ss, ax
    mov sp, 0x7c00
    ;print hello message
    mov si, msg_hello
    call puts
    hlt

.halt:
    jmp .halt


msg_hello :db 'Hello, World!',Endl, 0

times 510-($-$$) db 0
dw 0xAA55
