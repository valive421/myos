bits 32

section .text

global task_context_switch

; void task_context_switch(task_context_t* from, task_context_t* to)
; task_context_t layout:
;   +0  esp
;   +4  ebp
;   +8  ebx
;   +12 esi
;   +16 edi
;   +20 eip
; cdecl on i386

task_context_switch:
    mov eax, [esp + 4]    ; from
    mov edx, [esp + 8]    ; to

    ; Save current context into *from.
    lea ecx, [esp + 4]    ; ESP as seen by caller after a normal RET
    mov [eax + 0], ecx
    mov [eax + 4], ebp
    mov [eax + 8], ebx
    mov [eax + 12], esi
    mov [eax + 16], edi
    mov ecx, [esp]        ; return address of caller
    mov [eax + 20], ecx

    ; Load next context from *to.
    mov esp, [edx + 0]
    mov ebp, [edx + 4]
    mov ebx, [edx + 8]
    mov esi, [edx + 12]
    mov edi, [edx + 16]
    mov ecx, [edx + 20]

    jmp ecx
