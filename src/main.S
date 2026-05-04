.code16

.section .text
start:
    mov $msg, %si
    call print_str
halt:
    hlt
    jmp halt

print_str:
    mov $0x0e, %ah
.loop:
    lodsb
    or %al, %al
    jz .done
    int $0x10
    jmp .loop
.done:
    ret

.section .data
msg:
    .asciz "Hello from .data!"
counter:
    .word 0x1234
