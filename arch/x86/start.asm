    [bits 32]
SECTION .start

    PAGE_SIZE         equ 0x1000
    PHY_START_ADDRESS equ 0x100000 ;1MB not used
    PHY_SIZE          equ 0x100000 * 127 ;127MB
    KERNEL_SIZE       equ 0x400000 ;4MB kernel
    KERNEL_PHY_START  equ 0x100000
    KERNEL_PHY_END    equ PHY_START_ADDRESS + KERNEL_SIZE



    VGA_PHY_START     equ 0xb8000
    VGA_VMM_START     equ 0xc0000000 + KERNEL_SIZE

    PET_ENTRY_SIZE          equ 4
    PET_TABLE_SIZE          equ PAGE_SIZE
    PDT_TABLE_START         equ KERNEL_PHY_END + PAGE_SIZE
    PDT_TABLE_SIZE          equ PAGE_SIZE
    PDT_ENTRY_SIZE          equ 4
    PET_TABLES_START        equ PDT_TABLE_START + PAGE_SIZE
    PER_PET_PAGE_MM_SIZE    equ 0x400000
    PER_PDT_ENTRY_MM_SIZE   equ PER_PET_PAGE_MM_SIZE

    VMM_START_ADDRESS   equ 0xc0000000

    GDT_BASE equ VMM_START_ADDRESS + KERNEL_SIZE - PAGE_SIZE
    IDT_BASE equ GDT_BASE - PAGE_SIZE

    INIT_STACK_SIZE equ 4096 * 2

global gdt_size
global gdt_tables

extern kernel_start
extern init_stack_space
extern __bss_start
extern __bss_end

    ;; we already in real mode now
    ;; and we are in 0x100000
__start:
    ;; 0. init temp sp
    mov esp, 0x100000
    ;; 1. init kernel page table
    ;; eax = pdr address
    jmp  init_mmu_table
init_mmu_ok:
    ;; set cr3 and open PG
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    ;; 2. init new gdt
    jmp init_gdt_table
init_gdt_ok:
    ;; lgdt
    lgdt [gdt_size]

init_idt_base:
    ;; we just init idtr, but not init the idt entry
    ;; we will do this in c code later
    mov eax, idt_size
    mov ebx, 256 * 8 - 1
    mov [eax], ebx
    mov ebx, IDT_BASE
    mov [eax + 2], ebx
    lidt [idt_size]

init_sp:    ;; init sp
    mov esp, init_stack_space
    add esp, INIT_STACK_SIZE

    xor eax, eax
    mov ebx, __bss_start
    mov ecx, __bss_end
    cmp ebx, ecx
    je jmp_to_kernel
init_bss:
    mov [ebx], eax
    add ebx, 4
    cmp ebx, ecx
    jne init_bss

jmp_to_kernel:
    mov eax, kernel_start
    jmp eax

    ;; -------------------------------------------------------------------------------------------
init_mmu_table:
    ;;init for PDT

    push eax
    push ebx
    push ecx
    push edx

    mov ebx, PDT_TABLE_START + (VMM_START_ADDRESS / PER_PDT_ENTRY_MM_SIZE) * PDT_ENTRY_SIZE
    mov eax, PET_TABLES_START
    add eax, 0x3
    mov ecx, PHY_SIZE / PER_PDT_ENTRY_MM_SIZE;how many PET page we need
pdt_high_init:
    mov [ebx], eax
    add eax, PAGE_SIZE
    add ebx, PDT_ENTRY_SIZE
    dec ecx
    jnz pdt_high_init

    mov ebx, PDT_TABLE_START
    mov eax, (PHY_SIZE / PER_PDT_ENTRY_MM_SIZE) * PAGE_SIZE + PET_TABLES_START
    add eax, 0x3
    mov ecx, KERNEL_PHY_END / PER_PDT_ENTRY_MM_SIZE + 1

pdt_low_init:
    mov [ebx], eax
    add eax, PAGE_SIZE
    add ebx, PDT_ENTRY_SIZE
    dec ecx
    jnz pdt_low_init

    mov ebx, PET_TABLES_START
    mov eax, PHY_START_ADDRESS
    add eax, 0x3
    mov ecx, PHY_SIZE / PAGE_SIZE
pet_high_init:
    mov [ebx], eax
    add eax, PAGE_SIZE
    add ebx, PET_ENTRY_SIZE
    dec ecx
    jnz pet_high_init


    mov ebx, (PHY_SIZE / PER_PDT_ENTRY_MM_SIZE) * PET_TABLE_SIZE + PET_TABLES_START
    mov eax, 0
    add eax, 0x3
    mov ecx, KERNEL_PHY_END / PAGE_SIZE
pet_low_init:
    mov [ebx], eax
    add eax, PAGE_SIZE
    add ebx, PET_ENTRY_SIZE
    dec ecx
    jnz pet_low_init

vga_ptb_init:
    mov ebx, PET_TABLES_START + (KERNEL_SIZE / PAGE_SIZE) * PET_ENTRY_SIZE
    mov eax, VGA_PHY_START
    add eax, 0x3
    mov [ebx], eax




    pop edx
    pop ecx
    pop ebx
    pop eax
    mov eax, PDT_TABLE_START
    jmp init_mmu_ok

    ;; -------------------------------------------------------------------------------------
init_gdt_table:
    mov eax, GDT_BASE


    mov dword [eax], 0x00
    mov dword [eax + 4], 0x00

    mov dword [eax + 8], 0x00
    mov dword [eax + 12], 0x00

    ;; kernel code
    mov dword [eax + 16], 0x0000ffff
    mov dword [eax + 20], 0x00cf9a00

    ;; kernel data
    mov dword [eax + 24], 0x0000ffff
    mov dword [eax + 28], 0x00cf9200

    ;; user code
    mov dword [eax + 32], 0x0000ffff
    mov dword [eax + 36], 0x00cffa00

    ;; user data
    mov dword [eax + 40], 0x0000ffff
    mov dword [eax + 44], 0x00cff200

    ;; end
    mov dword [eax + 48], 0x00
    mov dword [eax + 52], 0x00


    mov [gdt_base], eax
    mov ax, 47
    mov [gdt_size], ax
    jmp init_gdt_ok

    gdt_size dw 0
    gdt_base dd 0

    idt_size dw 0
    idt_base dd 0
