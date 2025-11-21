global _start
_start:
    mov rax, 9
    push rax
    mov rax, 6
    push rax
    push QWORD [rsp + 0]
    push QWORD [rsp + 16]
    call func_add
    add rsp, 16
    push rax
    push QWORD [rsp + 16]
    mov rax, 6
    push rax
    call func_multip
    add rsp, 16
    push rax
    pop rax
    pop rbx
    add rax, rbx
    push rax
    pop rax
    mov [rsp + 18446744073709551608], rax
    push QWORD [rsp + 18446744073709551608]
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
    mov rax, 60
    mov rdi, 0
    syscall

; Function stream starting here
func_add:
    push rbp
    mov rbp, rsp
    push QWORD [rbp + 24]
    push QWORD [rbp + 16]
    pop rax
    pop rbx
    add rax, rbx
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
func_multip:
    push rbp
    mov rbp, rsp
    push QWORD [rbp + 24]
    push QWORD [rbp + 16]
    pop rax
    pop rbx
    mul rbx
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

