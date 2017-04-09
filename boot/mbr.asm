
    org 0x7c00

    xor ax,ax
    mov ss, ax
    mov sp, ax
    call clear_screen
    ;; load
    mov dl, 0x80                ;disk
    mov ch, 0x00
    mov cl, 2049
    mov dh, 0
    mov ax, 0x1000
    mov es, ax
    mov bx, 0
    mov al, 10
    mov ah, 0x02
    int 13h

    jmp 0x1000:0x0000

clear_screen:
    push ax
    push bx
    push cx
    push dx
    mov ax, 0x0600
    mov bx, 0x0700
    mov cx, 0
    mov dx, 0xffff
    int 0x10
    pop dx
    pop cx
    pop bx
    pop ax
    ret
target_address: dd 0x10000
    times 510 - ($ - $$) db 0
mag:    dw 0xaa55
