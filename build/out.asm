global _start
_start:
    mov rax, 2
    push rax
    mov rax, 10
    push rax
    mov rax, 2
    push rax
    push QWORD [rsp + 16]
    pop rax
    pop rbx
    sub rax, rbx
    push rax
    pop rax
    test rax, rax
    jz label0
    mov rax, 5
    push rax
    mov rax, 60
    pop rdi
    syscall
    add rsp, 0
    jmp label1
label0:
    ;; else if
    mov rax, 10
    push rax
    push QWORD [rsp + 8]
    pop rax
    pop rbx
    sub rax, rbx
    push rax
    pop rax
    test rax, rax
    jz label2
    mov rax, 4
    push rax
    mov rax, 60
    pop rdi
    syscall
    add rsp, 0
    jmp label0
label2:
    ;; else
    mov rax, 3
    push rax
    push QWORD [rsp + 8]
    pop rax
    pop rbx
    mul rbx
    push rax
    mov rax, 1
    push rax
    pop rax
    pop rbx
    add rax, rbx
    push rax
    mov rax, 60
    pop rdi
    syscall
    add rsp, 0
label1:
    mov rax, 60
    mov rdi, 0
    syscall