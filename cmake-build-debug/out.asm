global _start
_start:
    mov rax, 11
    push rax
    mov rax, 1
    push rax
    mov rax, 1
    push rax
    mov rax, 0
    push rax
label0:
    push QWORD [rsp + 24]
    pop rax
    test rax, rax
    jz label1
    mov rax, 1
    push rax
    push QWORD [rsp + 32]
    pop rax
    pop rbx
    sub rax, rbx
    push rax
    pop rax
    mov [rsp + 24], rax 
    push QWORD [rsp + 0]
    push QWORD [rsp + 16]
    pop rax
    pop rbx
    add rax, rbx
    push rax
    pop rax
    mov [rsp + 16], rax 
    push QWORD [rsp + 0]
    pop rax
    mov [rsp + 8], rax 
    push QWORD [rsp + 16]
    pop rax
    mov [rsp + 0], rax 
    add rsp, 0
    jmp label0
label1:
    push QWORD [rsp + 16]
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