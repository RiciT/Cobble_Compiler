global _start
_start:
    mov rax, 1
    push rax
    mov rax, 0
    push rax
    mov rax, 5
    push rax
    mov rax, 10
    push rax
    push QWORD [rsp + 8]
    push QWORD [rsp + 8]
    pop rbx
    pop rax
    cmp rax, rbx
    setl al
    movzx rax, al
    push rax
    push QWORD [rsp + 0]
    pop rax
    test rax, rax
    jz label0
    mov rax, 1
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
    jnz label1
    mov BYTE [rdi], '0'
    dec rdi
    inc rcx
    jmp label2
label1:
    test rax, rax
    jz label2
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    inc rcx
    jmp label1
label2:
    inc rdi
    mov rax, 1
    mov rsi, rdi
    mov rdi, 1
    mov rdx, rcx
    syscall
    add rsp, 32
    add rsp, 0
label0:
    mov rax, 60
    mov rdi, 0
    syscall

; Function stream starting here

