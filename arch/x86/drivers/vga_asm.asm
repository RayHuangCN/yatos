    [bits 32]
SECTION .text
    global vga_get_cursor
    global vga_set_cursor
vga_get_cursor:
    ;; get screen position
    push edx

    xor eax, eax

    mov dx, 0x3d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x3d5
    in  al, dx
    mov ah, al

    mov dx, 0x3d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x3d5
    in al, dx

    pop edx
    ret

vga_set_cursor:
    push ebp
    mov ebp, esp
    push eax
    push edx

    mov eax, [ebp + 8]

    mov dx,0x3d4
    mov al,0x0e
    out dx,al
    mov dx,0x3d5
    mov al,bh
    out dx,al
    mov dx,0x3d4
    mov al,0x0f
    out dx,al
    mov dx,0x3d5
    mov al,bl
    out dx,al

    pop edx
    pop eax
    pop ebp
    ret
