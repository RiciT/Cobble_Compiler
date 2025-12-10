global _start
_start:
    mov rax, 155
    push rax
    push QWORD [rsp + 0]
    mov rax, 1
    push rax
    push QWORD [rsp + 8]
    pop rax
    pop rbx
    mul rbx
    push rax
    pop rax
    mov [rsp + 0], rax
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
label2:
    push QWORD [rsp + 0]
    mov rax, 150
    push rax
    pop rbx
    pop rax
    cmp rax, rbx
    setne al
    movzx rax, al
    push rax
    pop rax
    test rax, rax
    jz label3
    push QWORD [rsp + 8]
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
    jnz label4
    mov BYTE [rdi], '0'
    dec rdi
    inc rcx
    jmp label5
label4:
    test rax, rax
    jz label5
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    inc rcx
    jmp label4
label5:
    inc rdi
    mov rax, 1
    mov rsi, rdi
    mov rdi, 1
    mov rdx, rcx
    syscall
    add rsp, 32
    mov rax, 1
    push rax
    push QWORD [rsp + 8]
    pop rax
    pop rbx
    sub rax, rbx
    push rax
    pop rax
    mov [rsp + 0], rax
label6:
    push QWORD [rsp + 0]
    mov rax, 150
    push rax
    pop rbx
    pop rax
    cmp rax, rbx
    setne al
    movzx rax, al
    push rax
    pop rax
    test rax, rax
    jz label7
    push QWORD [rsp + 8]
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
    mov rax, 1
    push rax
    push QWORD [rsp + 8]
    pop rax
    pop rbx
    sub rax, rbx
    push rax
    pop rax
    mov [rsp + 0], rax
    add rsp, 0
    jmp label6
label7:
    add rsp, 0
    jmp label2
label3:
    push QWORD [rsp + 8]
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
    mov rax, 60
    mov rdi, 0
    syscall


; Functions
