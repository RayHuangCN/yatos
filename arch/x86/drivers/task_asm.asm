    [bits 32]
SECTION .text
    global task_arch_launch

task_arch_launch:
    push ebp
    mov ebp, esp
    mov ax, 0x2B
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ;; set up iret stack content
    push 0x2b                   ;ss
    mov eax, [ebp + 12]         ;esp
    push eax
    push 0x202                  ;eflags
    push 0x23                   ;cs
    mov eax, [ebp + 8]          ;eip
    push eax

    ;; goto user space
    iret
