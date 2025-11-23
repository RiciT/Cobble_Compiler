global _start
_start:
    sub rsp, 40
    mov rax, 0
    push rax
    mov rax, 4
    push rax
    pop rcx
    pop rax
    mov rbx, 8
    mul rbx
    mov rbx, rsp
    add rbx, 0
    add rbx, rax
    mov [rbx], rcx
    mov rax, 3
    push rax
    mov rax, 2
    push rax
    pop rax
    pop rbx
    mul rbx
    push rax
    mov rax, 5
    push rax
    pop rax
    mov [rsp + 0], rax
    mov rax, 2
    push rax
    push QWORD [rsp + 8]
    pop rax
    pop rbx
    sub rax, rbx
    push rax
    mov rax, 5
    push rax
    pop rcx
    pop rax
    mov rbx, 8
    mul rbx
    mov rbx, rsp
    add rbx, 8
    add rbx, rax
    mov [rbx], rcx
    mov rax, 19
    push rax
    push QWORD [rsp + 0]
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
    add rsp, 8
    mov rax, 5
    push rax
    mov rax, 5
    push rax
    mov rax, 10
    push rax
    pop rbx
    pop rax
    cmp rax, rbx
    setne al
    movzx rax, al
    push rax
    mov rax, 1
    push rax
    call func_ak
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
    mov rax, 3
    push rax
    pop rax
    mov rbx, 8
    mul rbx
    mov rbx, rsp
    add rbx, 16
    add rbx, rax
    mov rax, [rbx]
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
    jnz label5
    mov BYTE [rdi], '0'
    dec rdi
    inc rcx
    jmp label6
label5:
    test rax, rax
    jz label6
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    inc rcx
    jmp label5
label6:
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
func_ak:
    push rbp
    mov rbp, rsp
    push QWORD [rbp + 24]
    pop rax
    test rax, rax
    jz label2
    mov rax, 1
    push rax
    pop rax
    mov rsp, rbp
    pop rbp
    ret
    add rsp, 0
label2:
    mov rax, 100000
    push rax
    pop rax
    mov rsp, rbp
    pop rbp
    ret
    add rsp, 0
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret
