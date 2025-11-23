global _start
_start:
    sub rsp, 40
    mov rax, 0
    push rax
    mov rax, 42
    push rax
    pop rcx
    pop rax
    mov rbx, 8
    mul rbx
    mov rbx, rsp
    add rbx, 0
    add rbx, rax
    mov [rbx], rcx
    mov rax, 4
    push rax
    mov rax, 5
    push rax
    pop rcx
    pop rax
    mov rbx, 8
    mul rbx
    mov rbx, rsp
    add rbx, 0
    add rbx, rax
    mov [rbx], rcx
    mov rax, 9
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
    mov rax, 4
    push rax
    pop rax
    mov rbx, 8
    mul rbx
    mov rbx, rsp
    add rbx, 0
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
    mov rax, 60
    mov rdi, 0
    syscall


; Functions
