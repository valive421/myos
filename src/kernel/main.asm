
bits 16
org 0

CODE_SEL equ 0x08
DATA_SEL equ 0x10
VIDEO_SEL equ 0x18

start:
    jmp main

rm_puts:
    push si
    push ax

.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp .loop

.done:
    pop ax
    pop si
    ret

gdt_start:
gdt_null:
    dq 0

gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_flat:
    dw 0xFFFF
    dw 0x8000
    db 0x0B
    db 10010010b
    db 01000000b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd 0

main:
    ; Set up segment registers for current load segment
    mov ax, cs
    mov ds, ax
    mov es, ax

    ; Set up stack in current segment
    mov ss, ax
    mov sp, 0xFFFE

    mov si, msg_hello_rm
    call rm_puts

    cli

    ; Compute current segment physical base (cs << 4)
    xor eax, eax
    mov ax, cs
    shl eax, 4

    ; Patch code/data descriptor base to current load base
    mov [gdt_code + 2], ax
    mov [gdt_data + 2], ax
    shr eax, 16
    mov [gdt_code + 4], al
    mov [gdt_data + 4], al
    mov byte [gdt_code + 7], 0
    mov byte [gdt_data + 7], 0

    ; Patch GDTR base with physical address of GDT
    xor ebx, ebx
    mov bx, cs
    shl ebx, 4
    add ebx, gdt_start
    mov [gdt_descriptor + 2], ebx

    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEL:protected_mode_entry

[bits 32]
protected_mode_entry:
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov ax, VIDEO_SEL
    mov fs, ax

    mov esp, 0x0000FF00

    mov esi, msg_hello_pm
    xor edi, edi
    mov ah, 0x0F

.print_loop:
    lodsb
    test al, al
    jz .halt

    mov [fs:edi], al
    mov [fs:edi + 1], ah
    add edi, 2
    jmp .print_loop

.halt:
    hlt

msg_hello_rm: db 'Hello from kernel Vaibhav (real mode).', 0x0D, 0x0A, 0
msg_hello_pm: db 'Hello from protected mode kernel Vaibhav.', 0
