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
    mov rax, 60
    mov rdi, 0
    syscall


; Functions
func_x:
    push rbp
    mov rbp, rsp
    push QWORD [rbp + 16]
    pop rax
    mov rsp, rbp
    pop rbp
    ret
    add rsp, 0
    mov rax, 0
    mov rsp, rbp
    pop rbp
    ret
