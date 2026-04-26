# Primero estamos en modo real
.code16                         
.global _start
_start:
    cli # deshabilitamos interrupciones                        

    lgdt gdt_descriptor  # Cargamos la GDT


    mov %cr0, %eax  #Copiar CR0 a eax para modificarlo
    orl $0x1, %eax  #bit 0 - PE (protection enable) --> en 1 esta en modo protegido
    mov %eax, %cr0  #En este momento exacto es cuando cambia de modo real a modo protegido

    ljmp $CODE_SEG, $protected_mode  # saltamos a codigo de 32 bits para el modo protegido

# GDT  (Global Descriptor Table)

.align 8 #el procesador accede a la gdt mas eficientemente cuando esta alineado a 8 bytes
gdt_start:
    # Descriptor 0 nulo --> 8 bytes en cero
    .long 0x00000000
    .long 0x00000000

    # Descriptor 1 codigo --> vive el codigo del programa
    .word 0xFFFF                # limite 0:15
    .word 0x0000                # base 0:15
    .byte 0x00                  # base 16:23
    .byte 0x9A                  # acceso: codigo, ring 0, ejecutable
    .byte 0xCF                  # flags: 32 bits, granularidad 4KB
    .byte 0x00                  # base 24:31

    # Descriptor 2 datos --> aca viven los datos del programa
    .word 0xFFFF                # limite 0:15
    .word 0x0000                # base 0:15
    .byte 0x00                  # base 16:23
    .byte 0x92                  # acceso: datos, ring 0, lectura/escritura
    .byte 0xCF                  # flags: 32 bits, granularidad 4KB
    .byte 0x00                  # base 24:31
gdt_end:

# Estructura que carga lgdt --> Cargo la GDT
gdt_descriptor:
    .word gdt_end - gdt_start - 1   # tamaño de la GDT
    .long gdt_start                  # dirección de la GDT en memoria

# Constantes para referenciar los segmentos
.equ CODE_SEG, 0x08     # 2do descriptor = 8 bytes desde inicio
.equ DATA_SEG, 0x10     # 3er descriptor = 16 bytes desde inicio

# ahora pasamos a MODO PROTEGIDO - 32 bits

.code32
protected_mode:
    # 5. Configurar registros de segmento con DATA_SEG
    mov $DATA_SEG, %ax    # ax = 0x10 (índice del descriptor de datos)
    mov %ax, %ds          # DS = segmento de datos
    mov %ax, %es          # ES = segmento extra
    mov %ax, %fs          # FS = segmento extra 2
    mov %ax, %gs          # GS = segmento extra 3
    mov %ax, %ss          # SS = segmento de stack

    # Escribir "PROTECTED" en pantalla via memoria VGA (0xB8000)
    movb $'P', 0xB8000
    movb $0x0F, 0xB8001
    movb $'R', 0xB8002
    movb $0x0F, 0xB8003
    movb $'O', 0xB8004
    movb $0x0F, 0xB8005
    movb $'T', 0xB8006
    movb $0x0F, 0xB8007
    movb $'E', 0xB8008
    movb $0x0F, 0xB8009
    movb $'C', 0xB800A
    movb $0x0F, 0xB800B
    movb $'T', 0xB800C
    movb $0x0F, 0xB800D
    movb $'E', 0xB800E
    movb $0x0F, 0xB800F
    movb $'D', 0xB8010
    movb $0x0F, 0xB8011

    hlt                         # detener el procesador
