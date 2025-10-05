#pragma once

class Generator
{
public:
  inline explicit Generator(NodeExit root) : m_root(std::move(root))
  {
  }

  [[nodiscard]] std::string generate() const
  {
    std::stringstream output;
    output << "global _start\n";
    output << "section .text\n";
    output << "    align 16\n";
    output << "_start:\n";
    
    // Compute the expression result in rdi
    if (m_root.expr.add_op.has_value()) {
      output << "    mov rax, " << m_root.expr.int_lit.value.value_or("0") << "  ; first number\n";
      output << "    add rax, " << m_root.expr.int_lit2.value().value.value_or("0") << "  ; add second number\n";
    } else {
      output << "    mov rax, " << m_root.expr.int_lit.value.value_or("0") << "  ; load number\n";
    }
    
    // Convert number to string and print it
    output << "\n    ; Convert number in rax to string and print\n";
    output << "    mov rdi, rax          ; save the result\n";
    output << "    lea rsi, [rel buffer + 19]  ; point to end of buffer\n";
    output << "    mov byte [rsi], 10    ; newline\n";
    output << "    dec rsi\n";
    output << "    mov rbx, 10           ; divisor\n";
    output << "\n.convert_loop:\n";
    output << "    xor rdx, rdx          ; clear rdx for division\n";
    output << "    div rbx               ; divide rax by 10\n";
    output << "    add dl, '0'           ; convert remainder to ASCII\n";
    output << "    mov [rsi], dl         ; store digit\n";
    output << "    dec rsi               ; move backwards\n";
    output << "    test rax, rax         ; check if quotient is 0\n";
    output << "    jnz .convert_loop     ; if not, continue\n";
    output << "\n    ; Print the number\n";
    output << "    inc rsi               ; adjust to first digit\n";
    output << "    mov rax, 0x2000004    ; write syscall\n";
    output << "    mov rdi, 1            ; stdout\n";
    output << "    lea rdx, [rel buffer + 20]  ; end of buffer\n";
    output << "    sub rdx, rsi          ; calculate length\n";
    output << "    mov rdi, 1            ; stdout (rdi was overwritten)\n";
    output << "    syscall\n";
    
    // Exit with 0
    output << "\n    ; Exit\n";
    output << "    mov rax, 0x2000001    ; exit syscall\n";
    output << "    xor rdi, rdi          ; exit code 0\n";
    output << "    syscall\n\n";
    
    output << "section .bss\n";
    output << "    buffer: resb 20       ; buffer for number conversion\n";
    
    return output.str();
  }

private:
  const NodeExit m_root;
};