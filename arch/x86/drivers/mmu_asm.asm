    [bits 32]
    SECTION .text
    global mmu_page_fault_addr
    global mmu_set_page_table
    global mmu_flush
mmu_page_fault_addr:
    mov eax, cr2
    ret
mmu_set_page_table:
    push ebp
    mov ebp, esp

    push eax

    mov eax, [ebp]
    mov cr3, eax

    pop eax
    pop ebp

    ret

mmu_flush:
    push eax
    mov eax, cr3
    mov cr3, eax
    pop eax
    ret
