global _start
_start:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov rax, 155
    mov QWORD [rbp - 8], rax
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
L0_exit:
    mov rax, [rbp - 8]
    mov rbx, 155
    cmp rax, rbx
    setne al
    movzx rax, al
    mov QWORD [rbp - 16], rax
    mov rax, [rbp - 16]
    test rax, rax
    jnz L1_enter
L1_exit:
    mov rax, [rbp - 8]
    mov rbx, 155
    cmp rax, rbx
    sete al
    movzx rax, al
    mov QWORD [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jnz L2_enter
L2_exit:
L3_exit:
    mov rax, [rbp - 8]
    mov rbx, 150
    cmp rax, rbx
    setne al
    movzx rax, al
    mov QWORD [rbp - 32], rax
    mov rax, [rbp - 32]
    test rax, rax
    jnz L3_enter
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
L3_enter:
    mov rax, [rbp - 8]
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
    mov rax, [rbp - 8]
    sub rax, 1
    mov [rbp - 8], rax
    mov rax, [rbp - 8]
    mov rbx, 150
    cmp rax, rbx
    setne al
    movzx rax, al
    mov QWORD [rbp - 40], rax
    mov rax, [rbp - 40]
    test rax, rax
    jnz L3_enter
    jmp L3_exit
L2_enter:
    mov rax, [rbp - 8]
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
    jmp L2_exit
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
    jnz print8
    mov BYTE [rdi], '0'
    dec rdi
    inc rcx
    jmp print9
print8:
    test rax, rax
    jz print9
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    inc rcx
    jmp print8
print9:
    inc rdi
    mov rax, 1
    mov rsi, rdi
    mov rdi, 1
    mov rdx, rcx
    syscall
    add rsp, 32
    jmp L1_exit
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
    jnz print10
    mov BYTE [rdi], '0'
    dec rdi
    inc rcx
    jmp print11
print10:
    test rax, rax
    jz print11
    xor rdx, rdx
    div rbx
    add dl, '0'
    mov [rdi], dl
    dec rdi
    inc rcx
    jmp print10
print11:
    inc rdi
    mov rax, 1
    mov rsi, rdi
    mov rdi, 1
    mov rdx, rcx
    syscall
    add rsp, 32
    jmp L0_exit


; Functions
