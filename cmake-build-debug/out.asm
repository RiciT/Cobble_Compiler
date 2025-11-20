global _start
_start:
    mov rax, 5
    push rax
func_newFunc:
    push rbp
    mov rbp, rsp
    mov rax, 9
    push rax
    pop rax
    ; Convert integer in rax to ASCII
    mov rbx, 10
    mov rcx, 0
    sub rsp, 32
    mov rdi, rsp
    add rdi, 31
    mov BYTE [rdi], 10
    dec rdi
    inc rcx
    test rax, rax
    jnz .convert_loop
    mov BYTE [rdi], '0'
    dec rdi
    inc rcx
    jmp .done_convert
.convert_loop:
    test rax, rax
    jz .done_convert
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    inc rcx
    jmp .convert_loop
.done_convert:
    inc rdi
    mov rax, 1
    mov rsi, rdi
    mov rdi, 1
    mov rdx, rcx
    syscall
    add rsp, 32
    mov rax, 60
    mov rdi, 0
    syscall