org 0x7c00
bits 16


%define Endl 0x0D, 0x0A

;
;fat 12 HEADER 
;
JMP SHORT start
nop
bdb_oem :      db 'MSWIN4.1'
bdb_bytes_per_sector:  dw 512
bdb_sectors_per_cluster: db 1
bdb_reserved_sectors:  dw 1
bdb_nfat_count:    db 2
bdb_dir_entries_count:  dw 0E0h
bdb_total_sectors: dw 2880
bdb_media_descriptor_type:  db 0F0h
bdb_sectors_per_fat:   dw 9
bdb_sectors_per_track: dw 18
bdb_heads:   dw 2
bdb_hidden_sectors:    dd 0
bdb_large_sector_count: dd 0

;extended boot record
ebr_drive_number:  db 0
                   db 0 
ebr_signature:    db 29h
ebr_volume_id:    dd 12345678h
ebr_volume_label: db 'valive OS  '
ebr_fsystem_id: db 'FAT12   '

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

    ;read some sectors from disk
    mov [ebr_drive_number], dl ;store drive number
    mov ax,1
    mov cl,1
    mov bx,0x7E00
    call disk_read


    ;print hello message
    mov si, msg_hello
    call puts
    cli
    hlt




;
;floppy error
;
floppy_error:
    mov si, msg_read_failed
    call puts
    jmp wait_key_press

wait_key_press:
    mov ah, 0
    int 16h
    jmp 0ffffh:0 ;reboot system
.halt:
    cli
    jmp .halt
;
;Disk routines would go here
;


;
;lba to chs address conversion
;
lba_to_chs:
    push ax
    push dx

    xor ax, ax ; clear ax
    div word[bdb_sectors_per_track] ; ax = lba / sectors per track
                          ; dx = lba % sectors per track          
    inc dx ; dx = (lba % sectors per track) + 1 
    mov cx, dx ; cx = sector number

    xor dx, dx ; clear dx 
    div word[bdb_heads] ; ax =(lba / sectors per track) / heads = cylinder
                         ; dx = (lba / sectors per track) % heads = head
    xor dx, dx ; clear dx, use dl as needed
    mov ch, al ; ch = cylinder number
    mov al, ch
    and al, 0xC0 ; keep only the top two bits of cylinder in al
    or ah, al    ; ah |= (cylinder number & 0xC0)
    or ah, cl    ; ah |= sector number
    or ah, cl  ; ah = sector number | (cylinder number & 0xC0)
    
    pop ax
    mov dl , al ; return head in dl
    pop ax
    ret

;
;read sector from disk
;
disk_read :
    push ax
    push bx
    push cx
    push dx
    push di 
    
    push cx
    call lba_to_chs
    pop ax
    mov ah, 02h ; BIOS read sector function
    int 13h   ; call BIOS interrupt
    mov di, 3
.retry :
    pusha
    stc
    int 13h
    jnc .done
    
    ;read failed, retry
    popa
    call disk_reset
    dec di
    test di, di
    jnz .retry
.fail:
    jmp floppy_error    

.done :
    popa
    
    pop di 
    pop dx
    pop cx
    pop bx
    pop ax
    
    ret

;
;
;disk reset
disk_reset:
    pusha
    mov ah, 0 ; BIOS reset function
    stc
    int 13h
    jc floppy_error
    popa
    ret

    
msg_hello :db 'Hello, World!',Endl, 0
msg_read_failed :db 'Floppy Read Error!',Endl, 0
times 510-($-$$) db 0
dw 0xAA55
