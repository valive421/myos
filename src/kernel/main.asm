bits 16

section .text

global start
global kernel_base_phys
extern kmain

CODE_SEL equ 0x08
DATA_SEL equ 0x10

start:
    cli

    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0xFFFE

    mov si, msg_hello_rm
    call rm_puts

    ; Enable A20 via port 0x92 (fast A20 gate)
    in al, 0x92
    or al, 0x02
    out 0x92, al

    ; Build temporary GDT base from current segment
    xor eax, eax
    mov ax, cs
    shl eax, 4
    mov [kernel_base_phys], eax

    mov [gdt_code + 2], ax
    mov [gdt_data + 2], ax
    shr eax, 16
    mov [gdt_code + 4], al
    mov [gdt_data + 4], al

    xor ebx, ebx
    mov bx, cs
    shl ebx, 4
    add ebx, gdt_start
    mov [gdt_descriptor + 2], ebx

    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEL:pm_entry

rm_puts:
    push ax
    push si

.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp .loop

.done:
    pop si
    pop ax
    ret

[bits 32]
pm_entry:
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    mov esp, 0x0000F000

    call kmain

.hang:
    cli
    hlt
    jmp .hang

align 8
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

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd 0

section .data

kernel_base_phys:
    dd 0

msg_hello_rm: db 'Hello from kernel (real mode).', 0x0D, 0x0A, 0
