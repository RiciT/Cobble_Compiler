global _start
_start:
    mov rax, 5
    push rax
    push QWORD [rsp + 0]
    push QWORD [rsp + 8]
    pop rax
    pop rbx
    add rax, rbx
    push rax
    pop rax
    mov [rsp + 0], rax
    push QWORD [rsp + 0]
    call func_x
    add rsp, 8
    call func_y
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
    mov rax, 60
    mov rdi, 0
    syscall


; Functions
func_x:
    push rbp
    mov rbp, rsp
    push QWORD [rbp + 16]
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
    mov rax, 5
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
func_y:
    push rbp
    mov rbp, rsp
    mov rax, 5
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
    jnz label2
    mov BYTE [rdi], '0'
    dec rdi
    inc rcx
    jmp label3
label2:
    test rax, rax
    jz label3
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    inc rcx
    jmp label2
label3:
    inc rdi
    mov rax, 1
    mov rsi, rdi
    mov rdi, 1
    mov rdx, rcx
    syscall
    add rsp, 32
    mov rax, 4
    push rax
    pop rax
    mov rsp, rbp
    pop rbp
    ret
    mov rax, 2
    push rax
    pop rax
    mov rsp, rbp
    pop rbp
    ret
    mov rax, 5
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
