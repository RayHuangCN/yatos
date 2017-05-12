    SECTION .text

global sig_ret_code_start
global sig_ret_code_end

sig_ret_code_start:
    mov eax, 42
    int 0x80
sig_ret_code_end:
    jmp sig_ret_code_end
