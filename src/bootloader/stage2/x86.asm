bits 16

section .text

global x86_div64_32
x86_div64_32:
    ; GCC -m16 cdecl stack layout (32-bit slots):
    ; [ebp +  8] = dividend low dword
    ; [ebp + 12] = dividend high dword
    ; [ebp + 16] = divisor
    ; [ebp + 20] = quotientOut (near pointer)
    ; [ebp + 24] = remainderOut (near pointer)

    push ebp
    mov ebp, esp
    push bx

    mov eax, [ebp + 12]
    mov ecx, [ebp + 16]
    xor edx, edx
    div ecx

    mov bx, [ebp + 20]
    mov [bx + 4], eax

    mov eax, [ebp + 8]
    div ecx

    mov [bx], eax
    mov bx, [ebp + 24]
    mov [bx], edx

    pop bx
    mov esp, ebp
    pop ebp
    retd

global x86_Video_WriteCharTeletype
x86_Video_WriteCharTeletype:
    ; [ebp + 8]  = character
    ; [ebp + 12] = page

    push ebp
    mov ebp, esp
    push bx

    mov ah, 0Eh
    mov al, [ebp + 8]
    mov bh, [ebp + 12]
    int 10h

    pop bx
    mov esp, ebp
    pop ebp
    retd

global x86_Disk_Reset
x86_Disk_Reset:
    ; [ebp + 8] = drive

    push ebp
    mov ebp, esp

    mov ah, 0
    mov dl, [ebp + 8]
    stc
    int 13h

    mov ax, 1
    sbb ax, 0

    mov esp, ebp
    pop ebp
    retd

global x86_Disk_Read
x86_Disk_Read:
    ; [ebp +  8] = drive
    ; [ebp + 12] = cylinder
    ; [ebp + 16] = sector
    ; [ebp + 20] = head
    ; [ebp + 24] = count
    ; [ebp + 28] = segment
    ; [ebp + 32] = offset

    push ebp
    mov ebp, esp

    push bx
    push es

    mov dl, [ebp + 8]

    mov ch, [ebp + 12]
    mov cl, [ebp + 13]
    shl cl, 6

    mov al, [ebp + 16]
    and al, 3Fh
    or cl, al

    mov dh, [ebp + 20]
    mov al, [ebp + 24]

    mov bx, [ebp + 28]
    mov es, bx
    mov bx, [ebp + 32]

    mov ah, 02h
    stc
    int 13h

    mov ax, 1
    sbb ax, 0

    pop es
    pop bx

    mov esp, ebp
    pop ebp
    retd

global x86_Disk_GetDriveParams
x86_Disk_GetDriveParams:
    ; [ebp +  8] = drive
    ; [ebp + 12] = driveTypeOut
    ; [ebp + 16] = cylindersOut
    ; [ebp + 20] = sectorsOut
    ; [ebp + 24] = headsOut

    push ebp
    mov ebp, esp

    push es
    push bx
    push si
    push di

    mov dl, [ebp + 8]
    mov ah, 08h
    xor di, di
    mov es, di
    stc
    int 13h

    mov ax, 1
    sbb ax, 0

    mov si, [ebp + 12]
    mov [si], bl

    mov bl, ch
    mov bh, cl
    shr bh, 6
    mov si, [ebp + 16]
    mov [si], bx

    xor ch, ch
    and cl, 3Fh
    mov si, [ebp + 20]
    mov [si], cx

    xor ch, ch
    mov cl, dh
    mov si, [ebp + 24]
    mov [si], cx

    pop di
    pop si
    pop bx
    pop es

    mov esp, ebp
    pop ebp
    retd

global x86_JumpToKernel
x86_JumpToKernel:
    ; [ebp +  8] = segment (uint16_t promoted to int)
    ; [ebp + 12] = offset  (uint16_t promoted to int)

    push ebp
    mov ebp, esp

    cli
    push word [ebp + 8]
    push word [ebp + 12]
    retf