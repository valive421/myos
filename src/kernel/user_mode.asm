bits 32

section .text

global task_enter_user_mode

; void task_enter_user_mode(uint32_t user_eip, uint32_t user_esp)
; Enters CPL=3 using an IRET privilege switch.
; Expects:
;   user_eip at [esp+4]
;   user_esp at [esp+8]
;
; Uses GDT selectors:
;   USER_CS = 0x1B
;   USER_DS = 0x23

task_enter_user_mode:
    mov eax, [esp + 4]        ; user_eip
    mov edx, [esp + 8]        ; user_esp

    mov bx, 0x23              ; USER_DS (RPL=3)
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    push dword 0x23           ; SS
    push edx                  ; ESP

    pushfd
    pop ecx
    or ecx, 0x00000200        ; IF=1
    and ecx, 0xFFFFCFFF       ; IOPL=0
    push ecx

    push dword 0x1B           ; CS
    push eax                  ; EIP
    iretd
