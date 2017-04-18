    [bits 32]
SECTION .text
    global task_arch_launch

task_arch_launch:
    push ebp
    mov ebp, esp

    mov eax, [ebp + 12]         ;change stack
    mov esp, eax
    mov eax, 0x202
    push eax                    ;eflag
    mov eax, 0x23               ;cs
    push eax
    mov eax, [ebp + 8]          ;eip
    push eax
    mov eax, 0x2b               ;ds
    iret
