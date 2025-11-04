global _start
_start:
    mov rax, 1
    push rax
    mov rax, 1
    push rax
    push QWORD [rsp + 8]
    pop rax
    pop rbx
    sub rax, rbx
    push rax
    pop rax
    test rax, rax
    jz label0
    mov rax, 4
    push rax
    pop rax
    mov [rsp + 0], rax 
    add rsp, 0
    jmp label1
label0:
    ;; else if
    mov rax, 1
    push rax
    push QWORD [rsp + 8]
    pop rax
    pop rbx
    sub rax, rbx
    push rax
    pop rax
    test rax, rax
    jz label2
    mov rax, 5
    push rax
    pop rax
    mov [rsp + 0], rax 
    add rsp, 0
    jmp label1
label2:
    ;; else
    mov rax, 6
    push rax
    pop rax
    mov [rsp + 0], rax 
    add rsp, 0
label1:
    push QWORD [rsp + 0]
    mov rax, 60
    pop rdi
    syscall
    mov rax, 60
    mov rdi, 0
    syscall