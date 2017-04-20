    [bits 32]
SECTION .text
    global task_arch_launch
    global task_arch_switch_to
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
    pushfd                  ;eflags
    pop eax
    or eax, 0x200
    push eax
    push 0x23                   ;cs
    mov eax, [ebp + 8]          ;eip
    push eax

    ;; goto user space
    iret



task_arch_switch_to:
    ;; task_arch_switch_to(struct task *pre, struct task *next);
    push ebp
    mov ebp, esp
    push eax
    push ebx
    push ecx

    mov eax, [ebp + 8]          ;pre
    mov ebx, [ebp + 12]         ;next;

    ;; save pre context
    mov ecx , switch_back
	push ecx                    ;eip
    pushfd
    push edx
    push ecx
    push ebx
    push eax
    push ebp
    push edi
    push esi

    ;; save pre->cur_stack
    mov [eax], esp

    ;; change to next->cur_stack;
    mov esp, [ebx]
    pop esi
    pop edi
    pop ebp
    pop eax
    pop ebx
    pop ecx
    pop edx
    popfd
    ret                         ;= pop eip
switch_back:

    pop ecx
    pop ebx
    pop eax
    pop ebp
    ret
