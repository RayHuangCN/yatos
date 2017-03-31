    ;; since we will into real mode,BIOS can not be used any more

    xor ax, ax
    mov ss, ax
    mov sp, 0x7c00 + 512

    ;; 0. set up GDT

    mov ax, 0
    mov ds, ax
    mov bx, 0x7e00

    ;; item 0, must be zero
    mov dword [bx + 0], 0x00
    mov dword [bx + 4], 0x00
    ;; item 1 we make it to be zero
    mov dword [bx + 8], 0x00
    mov dword [bx + 12], 0x00

    ;; item 2, 0-4G, code
    mov dword [bx + 16], 0x0000ffff
    mov dword [bx + 20], 0x00cf9a00

    ;; item 3, 0-4G, data
    mov dword [bx + 24], 0x0000ffff
    mov dword [bx + 28], 0x00cf9200

    ;; init gdt reg
    mov word [cs:gdt_size], 31
    mov eax, [cs:gdt_size]
    lgdt [cs:gdt_size]

    ;; 1. to readl mode

    ;; open A20
    in al, 0x92
    or al, 000_0010B
    out 0x92, al

    ;; disable interrupt
    cli


    ;; set PE
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ;; jmp to read mode
    jmp dword 0x010:flush+0x10000

    [bits 32]
flush:
    ;; 2. to load kernel
    mov cx, 0x18
    mov ds, cx
    mov ss, cx
    mov es, cx


    mov esp, 0x10000 + 0x1000


    mov eax, load_kernel_mes + 0x10000
    call put_string

    mov ecx, 1024
    mov ebx, 0x100000
    mov eax, 32
read_kernel:
    push ebx    ;buffer address
    push 0x0
    push 16            ;8kb a time
    push eax           ;start sector_number

    call _read_sectors_lba24

    pop edx
    pop edx
    pop edx
    pop edx

    add eax, 16
    add ebx, 512 * 2 * 8

    dec ecx
    jnz read_kernel

    ;; 3. start kernel
    mov eax, start_kernel_mes + 0x10000
    call put_string

    jmp  [kernel_address + 0x10000]

check_ready:
    mov  dx, 0x1f7
    in   al, dx
    test al, 0x40
    jz   check_ready
    ret

check_read_complete:
    mov  dx, 0x1f7
    in   al, dx
    test al, 0x08
    jz   check_ready
    ret

pio_delay:
    nop
    nop
    nop
    nop
    ret


    ;;
    ;;  read sectors to buffer
    ;;  void
    ;;  read_sectors_lba24 (
    ;;    int sector_number,
    ;;    int sector_count,
    ;;    int driver,
    ;;    int buffer);
    ;;
    ;;  parameters:
    ;;    sector_number: the start sector to read
    ;;    sector_count: the count of sectors to read
    ;;    driver: 0: 1st desk, 1: 2nd desk
    ;;    buffer: the address of buffer
    ;;
_read_sectors_lba24:
    push ebp
    mov  ebp, esp

    push eax
    push ebx
    push ecx
    push edx
    push edi

    call check_ready

    mov  dx, 0x1f2
    mov  eax, dword [ebp+12]
    out  dx, al         ; sector count
    call pio_delay

    mov  dx, 0x1f3
    mov  eax, dword [ebp+8]
    out  dx, al         ; 0~7 bit
    call pio_delay

    mov  dx, 0x1f4
    mov  eax, dword [ebp+8]
    shr  eax, 8         ; 8~15 bit
    out  dx, al
    call pio_delay

    mov  dx, 0x1f5
    mov  eax, dword [ebp+8]
    shr  eax, 16        ; 16~23 bit
    out  dx, al
    call pio_delay

    mov  ebx, dword [ebp+16]
    and  ebx, 1
    shl  bl, 4

    mov  al, 0xe0       ; 5~7: 111, 4: driver, 0~3: 24~27 bit
    or   al, bl
    mov  ebx, dword [ebp+8]
    shr  ebx, 24        ; 24~27 bit
    and  bl, 0x0f
    or   al, bl

    mov  dx, 0x1f6
    out  dx, al

    mov  dx, 0x1f7
    mov  al, 0x20       ; read until succeed
    out  dx, al
    call check_read_complete

    mov ecx, 256 * 16
    mov dx, 0x1f0
    mov ebx, dword [ebp + 20]
readw:
    in ax, dx
    mov [ebx], ax
    add ebx, 2
    loop readw


    pop  edi
    pop  edx
    pop  ecx
    pop  ebx
    pop  eax
    pop  ebp
    ret


put_string:
    mov ebx, [console_cur + 0x10000]
rep_str:
    mov cl, [eax]
    cmp cl, 0
    je ret_str

    mov [ebx], cl
    inc ebx
    mov cl, 1111_1100b
    mov [ebx], cl
    inc ebx
    inc eax

    jmp rep_str
ret_str:
    mov ebx, [console_cur + 0x10000]
    add ebx, 2*85
    mov [console_cur + 0x10000], ebx
    ret


;; gdt_infor----------------------------------------------------------
    gdt_size    dw 0
    gdt_base    dd 0x00007e00

    load_kernel_mes    db "loading kernel from disk......",0
    start_kernel_mes   db "starting kernel........" ,0

    console_cur  dd 0xb8000
    kernel_address dd 0x000100000
