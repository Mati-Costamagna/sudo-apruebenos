section .text
    global float_to_int
    global sumar_uno

; ---------------------------------------------------------
; Función: float_to_int
; Recibe: un float en el registro XMM0 (convención x86_64)
; Devuelve: un entero en el registro EAX
; ---------------------------------------------------------
float_to_int:
    ; --- PRÓLOGO ---
    push rbp            ; Salva el ancla de la capa anterior
    mov rbp, rsp        ; Establece el nuevo marco de pila (Stack Frame)

    ; --- CUERPO ---
    ; CVTTSS2SI: Convert with Truncation Scalar Single-precision to Signed Integer.
    ; Toma el valor de la API (en XMM0) y lo mete como entero en EAX.
    cvttss2si eax, xmm0 

    ; --- EPÍLOGO ---
    pop rbp             ; Restaura RBP
    ret                 ; Vuelve a la capa de C

; ---------------------------------------------------------
; Función: sumar_uno
; Recibe: un entero en el registro EDI (convención x86_64)
; Devuelve: el entero + 1 en el registro EAX
; ---------------------------------------------------------
sumar_uno:
    ; --- PRÓLOGO ---
    push rbp
    mov rbp, rsp

    ; --- CUERPO ---
    ; El entero que pasaste desde Python/C llega en EDI.
    mov eax, edi        ; Movemos el argumento al registro de retorno
    add eax, 1          ; Sumamos 1

    ; --- EPÍLOGO ---
    pop rbp
    ret
    
    section .note.GNU-stack noalloc noexec nowrite progbits