global _start
_start:
    mov rax, 155
    mov QWORD [rsp - 8], rax
    mov rax, 155
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
    jnz print0
    mov BYTE [rdi], '0'
    dec rdi
    inc rcx
    jmp print1
print0:
    test rax, rax
    jz print1
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    inc rcx
    jmp print0
print1:
    inc rdi
    mov rax, 1
    mov rsi, rdi
    mov rdi, 1
    mov rdx, rcx
    syscall
    add rsp, 32
    mov rax, 1
    test rax, rax
    jnz L0_enter
L1_exit:
    mov rax, [rsp - 8]
    mov rbx, 150
    cmp rax, rbx
    setne al
    movzx rax, al
    mov QWORD [rsp - 16], rax
    mov rax, [rsp - 16]
    test rax, rax
    jnz L1_enter
    mov rax, 155
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
    jnz print2
    mov BYTE [rdi], '0'
    dec rdi
    inc rcx
    jmp print3
print2:
    test rax, rax
    jz print3
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    inc rcx
    jmp print2
print3:
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
L1_enter:
    mov rax, 155
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
    jnz print4
    mov BYTE [rdi], '0'
    dec rdi
    inc rcx
    jmp print5
print4:
    test rax, rax
    jz print5
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    inc rcx
    jmp print4
print5:
    inc rdi
    mov rax, 1
    mov rsi, rdi
    mov rdi, 1
    mov rdx, rcx
    syscall
    add rsp, 32
    mov rax, [rsp - 8]
    sub rax, 1
    mov [rsp - 8], rax
    mov rax, [rsp - 8]
    mov rbx, 150
    cmp rax, rbx
    setne al
    movzx rax, al
    mov QWORD [rsp - 24], rax
    mov rax, [rsp - 24]
    test rax, rax
    jnz L1_enter
    jmp L1_exit
L0_exit:
L0_enter:
    mov rax, 155
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
    jnz print6
    mov BYTE [rdi], '0'
    dec rdi
    inc rcx
    jmp print7
print6:
    test rax, rax
    jz print7
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    inc rcx
    jmp print6
print7:
    inc rdi
    mov rax, 1
    mov rsi, rdi
    mov rdi, 1
    mov rdx, rcx
    syscall
    add rsp, 32
    jmp L0_exit


; Functions
