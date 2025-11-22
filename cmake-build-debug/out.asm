global _start
_start:
    sub rsp, 40
    mov rax, 0
    push rax
    mov rax, 100
    push rax
    pop rcx
    pop rax
    mov rbx, 8
    mul rbx
    mov rbx, rsp
    add rbx, 0
    add rbx, rax
    mov [rbx], rcx
    mov rax, 1
    push rax
    mov rax, 200
    push rax
    pop rcx
    pop rax
    mov rbx, 8
    mul rbx
    mov rbx, rsp
    add rbx, 0
    add rbx, rax
    mov [rbx], rcx
    mov rax, 2
    push rax
    mov rax, 300
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
    mov rax, 400
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
    mov rax, 500
    push rax
    pop rcx
    pop rax
    mov rbx, 8
    mul rbx
    mov rbx, rsp
    add rbx, 0
    add rbx, rax
    mov [rbx], rcx
    mov rax, 0
    push rax
label0:
    push QWORD [rsp + 0]
    mov rax, 5
    push rax
    pop rbx
    pop rax
    cmp rax, rbx
    setl al
    movzx rax, al
    push rax
    pop rax
    test rax, rax
    jz label1
    mov rax, 200
    push rax
    push QWORD [rsp + 8]
    pop rax
    mov rbx, 8
    mul rbx
    mov rbx, rsp
    add rbx, 16
    add rbx, rax
    mov rax, [rbx]
    push rax
    pop rax
    pop rbx
    xor rdx, rdx
    div rbx
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
    mov rax, 1
    push rax
    push QWORD [rsp + 8]
    pop rax
    pop rbx
    add rax, rbx
    push rax
    pop rax
    mov [rsp + 0], rax
    add rsp, 0
    jmp label0
label1:
    mov rax, 60
    mov rdi, 0
    syscall

; Function stream starting here

