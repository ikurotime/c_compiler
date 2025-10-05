global _start
section .text
    align 16
_start:
    mov rax, 12  ; first number
    add rax, 4  ; add second number

    ; Convert number in rax to string and print
    mov rdi, rax          ; save the result
    lea rsi, [rel buffer + 19]  ; point to end of buffer
    mov byte [rsi], 10    ; newline
    dec rsi
    mov rbx, 10           ; divisor

.convert_loop:
    xor rdx, rdx          ; clear rdx for division
    div rbx               ; divide rax by 10
    add dl, '0'           ; convert remainder to ASCII
    mov [rsi], dl         ; store digit
    dec rsi               ; move backwards
    test rax, rax         ; check if quotient is 0
    jnz .convert_loop     ; if not, continue

    ; Print the number
    inc rsi               ; adjust to first digit
    mov rax, 0x2000004    ; write syscall
    mov rdi, 1            ; stdout
    lea rdx, [rel buffer + 20]  ; end of buffer
    sub rdx, rsi          ; calculate length
    mov rdi, 1            ; stdout (rdi was overwritten)
    syscall

    ; Exit
    mov rax, 0x2000001    ; exit syscall
    xor rdi, rdi          ; exit code 0
    syscall

section .bss
    buffer: resb 20       ; buffer for number conversion
