global _start
_start:
    mov rax, 10
    push rax
label0:
    push QWORD [rsp + 0]
    pop rax
    test rax, rax
    jz label1
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
    jmp label0
label1:
    push QWORD [rsp + 0]
    pop rax
    test rax, rax
    jz label2
    mov rax, 4
    push rax
    pop rax
    mov [rsp + 0], rax 
    add rsp, 0
    jmp label3
label2:
    ;; else if
    push QWORD [rsp + 0]
    pop rax
    test rax, rax
    jz label4
    mov rax, 5
    push rax
    pop rax
    mov [rsp + 0], rax 
    add rsp, 0
    jmp label3
label4:
    ;; else
    mov rax, 6
    push rax
    pop rax
    mov [rsp + 0], rax 
    add rsp, 0
label3:
    push QWORD [rsp + 0]
    mov rax, 60
    pop rdi
    syscall
    mov rax, 60
    mov rdi, 0
    syscall