    .section .note.GNU-stack, "", @progbits
    .text

# ---------------------------------------------------------------
# int asm_float_to_int(6 long dummies, 8 double dummies, float gini_value)
#
# System V AMD64 ABI:
#   - 6 enteros en %rdi,%rsi,%rdx,%rcx,%r8,%r9
#   - 8 doubles en %xmm0-%xmm7
#   - El 9no float (gini_value) va al STACK
#
# Stack layout al entrar:
#   0x10(%rbp) = gini_value (float, en stack)
#
# Convierte float a int truncando y retorna en %eax.
# ---------------------------------------------------------------
    .globl asm_float_to_int
    .type  asm_float_to_int, @function
asm_float_to_int:
    pushq   %rbp
    movq    %rsp, %rbp

    movss   0x10(%rbp), %xmm0      # Cargar float desde el stack
    cvttss2si %xmm0, %eax          # Convertir float -> int (truncar)

    popq    %rbp
    ret


# ---------------------------------------------------------------
# int asm_sumar_uno(6 long dummies, int gini_int)
#
# System V AMD64 ABI:
#   - 6 enteros en %rdi,%rsi,%rdx,%rcx,%r8,%r9
#   - El 7mo entero (gini_int) va al STACK
#
# Stack layout al entrar:
#   0x10(%rbp) = gini_int (int, en stack)
#
# Suma 1 y retorna en %eax.
# ---------------------------------------------------------------
    .globl asm_sumar_uno
    .type  asm_sumar_uno, @function
asm_sumar_uno:
    pushq   %rbp
    movq    %rsp, %rbp

    movl    0x10(%rbp), %eax        # Cargar 7mo argumento desde el stack (32 bits)
    addl    $1, %eax                # Sumar 1

    popq    %rbp
    ret
