global _start
_start:
    mov rax, 5
    push rax
    call func_fib
    add rsp, 8
    push rax
    pop rax
    ;; Convert integer in rax to ASCII
    mov rbx, 10
    mov rcx, 0
    sub rsp, 32
    mov rdi, rsp
    add rdi, 31
    mov BYTE [rdi], 10
    dec rdi
    inc rcx
    test rax, rax
    jnz label0
    mov BYTE [rdi], '0'
    dec rdi
    inc rcx
    jmp label1
label0:
    test rax, rax
    jz label1
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    inc rcx
    jmp label0
label1:
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


; Functions
func_fib:
    push rbp
    mov rbp, rsp
    push QWORD [rbp + 16]
    pop rax
    mov rsp, rbp
    pop rbp
    ret
    add rsp, 0
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret
