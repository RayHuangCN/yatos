    [bits 32]
SECTION .text
    global pio_read
    global pio_write
    ;; int pio_read(address);
    ;; address port num
pio_read:
    push ebp
    mov  ebp, esp
    push edx

    xor eax, eax
    mov edx, [ebp + 8]
    in al, dx

    pop edx
    pop  ebp
    ret

    ;;void pio_write(value, address)
pio_write:
    push ebp
    mov ebp, esp

    push edx
    push eax

    mov eax, [ebp + 8]
    mov edx, [ebp + 12]

    out dx,al

    pop eax
    pop edx
    pop ebp
    ret
