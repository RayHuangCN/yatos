    [bits 32]
SECTION .text
    global pio_out8
    global pio_in8
    global pio_in16
    global pio_out16
    ;; int pio_read(address);
    ;; address port num
pio_in8:
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
pio_out8:
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
pio_out16:
    push ebp
    mov ebp, esp
    push edx
    push eax

    mov eax, [ebp + 8]
    mov edx, [ebp + 12]

    out dx, ax

    pop eax
    pop edx
    pop ebp
    ret

pio_in16:
    	push ebp
	    mov  ebp, esp
	    push edx

	    xor eax, eax
	    mov edx, [ebp + 8]
	    in ax, dx

	    pop edx
	    pop  ebp
	    ret
