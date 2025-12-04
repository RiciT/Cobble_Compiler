global _start
_start:
    mov rax, 0
    push rax
label5:
    push QWORD [rsp + 0]
    mov rax, 5
    push rax
    pop rbx
    pop rax
    cmp rax, rbx
    setne al
    movzx rax, al
    push rax
    pop rax
    test rax, rax
    jz label6
    push QWORD [rsp + 0]
    mov rax, 1
    push rax
    pop rbx
    pop rax
    cmp rax, rbx
    sete al
    movzx rax, al
    push rax
    pop rax
    test rax, rax
    jz label7
    mov rax, 555
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
    jnz label8
    mov BYTE [rdi], '0'
    dec rdi
    inc rcx
    jmp label9
label8:
    test rax, rax
    jz label9
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    inc rcx
    jmp label8
label9:
    inc rdi
    mov rax, 1
    mov rsi, rdi
    mov rdi, 1
    mov rdx, rcx
    syscall
    add rsp, 32
    add rsp, 0
label7:
    mov rax, 1
    push rax
    push QWORD [rsp + 8]
    pop rax
    pop rbx
    add rax, rbx
    push rax
    pop rax
    mov [rsp + 0], rax
    mov rax, 4
    push rax
    push QWORD [rsp + 8]
    call func_fib
    add rsp, 16
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
    jnz label10
    mov BYTE [rdi], '0'
    dec rdi
    inc rcx
    jmp label11
label10:
    test rax, rax
    jz label11
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    inc rcx
    jmp label10
label11:
    inc rdi
    mov rax, 1
    mov rsi, rdi
    mov rdi, 1
    mov rdx, rcx
    syscall
    add rsp, 32
    add rsp, 0
    jmp label5
label6:
    mov rax, 60
    mov rdi, 0
    syscall


; Functions
func_fib:
    push rbp
    mov rbp, rsp
    push QWORD [rbp + 16]
    push QWORD [rbp + 24]
    pop rbx
    pop rax
    cmp rax, rbx
    sete al
    movzx rax, al
    push rax
    pop rax
    test rax, rax
    jz label0
    push QWORD [rbp + 16]
    pop rax
    mov rsp, rbp
    pop rbp
    ret
    add rsp, 0
    jmp label1
label0:
    ;; else if
    mov rax, 2
    push rax
    push QWORD [rbp + 16]
    pop rax
    pop rbx
    add rax, rbx
    push rax
    push QWORD [rbp + 24]
    pop rbx
    pop rax
    cmp rax, rbx
    sete al
    movzx rax, al
    push rax
    pop rax
    test rax, rax
    jz label2
    push QWORD [rbp + 16]
    push QWORD [rbp + 24]
    pop rax
    pop rbx
    add rax, rbx
    push rax
    pop rax
    mov rsp, rbp
    pop rbp
    ret
    add rsp, 0
    jmp label1
label2:
label1:
    mov rax, 5
    push rax
    push QWORD [rbp + 24]
    push QWORD [rbp + 16]
    pop rax
    pop rbx
    add rax, rbx
    push rax
    pop rax
    pop rbx
    add rax, rbx
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
    jnz label3
    mov BYTE [rdi], '0'
    dec rdi
    inc rcx
    jmp label4
label3:
    test rax, rax
    jz label4
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    inc rcx
    jmp label3
label4:
    inc rdi
    mov rax, 1
    mov rsi, rdi
    mov rdi, 1
    mov rdx, rcx
    syscall
    add rsp, 32
    add rsp, 0
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret
