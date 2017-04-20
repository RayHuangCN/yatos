    [bits 32]
SECTION .text
    global sys_call_1
    global sys_call_2
    global sys_call_3
    global sys_call_4
    global sys_call_5

sys_call_1:
    push ebp
    mov ebp, esp

    mov eax, [ebp + 8]
    int 0x80

    pop ebp
    ret

sys_call_2:
    push ebp
    mov ebp, esp

    mov eax, [ebp + 8]
    mov ebx, [ebp + 12]
    int 0x80

    pop ebp
    ret

sys_call_3:
    push ebp
    mov ebp, esp

    mov eax, [ebp + 8]
    mov ebx, [ebp + 12]
    mov ecx, [ebp + 16]
    int 0x80

    pop ebp
    ret

sys_call_4:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    mov ebx, [ebp + 12]
    mov ecx, [ebp + 16]
    mov edx, [ebp + 20]
    int 0x80
    pop ebp
    ret
