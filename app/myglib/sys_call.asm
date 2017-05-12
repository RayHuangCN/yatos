    [bits 32]
SECTION .text
    global __sys_call_1
    global __sys_call_2
    global __sys_call_3
    global __sys_call_4
    global __sys_call_5

__sys_call_1:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    int 0x80
    pop ebp
    ret

__sys_call_2:
    push ebp
    mov ebp, esp
    push ebx

    mov eax, [ebp + 8]
    mov ebx, [ebp + 12]
    int 0x80

    pop ebx
    pop ebp
    ret

__sys_call_3:
    push ebp
    mov ebp, esp
    push ebx
    push ecx

    mov eax, [ebp + 8]
    mov ebx, [ebp + 12]
    mov ecx, [ebp + 16]
    int 0x80

    pop ecx
    pop ebx
    pop ebp
    ret

__sys_call_4:
    push ebp
    mov ebp, esp
    push ebx
    push ecx
    push edx

    mov eax, [ebp + 8]
    mov ebx, [ebp + 12]
    mov ecx, [ebp + 16]
    mov edx, [ebp + 20]
    int 0x80

    pop edx
    pop ecx
    pop ebx
    pop ebp
    ret
