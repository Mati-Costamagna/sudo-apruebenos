# Script GDB para depuración de aplicacion.efi sobre QEMU
# Uso: gdb -x debug.gdb
# Requisito previo: QEMU corriendo con -s -S (make run-debug)

# Conectar al servidor GDB de QEMU
target remote localhost:1234

# El firmware arranca en modo real de 16 bits
set architecture i8086

echo \n=== Firmware en modo real. Ejecutando 'continue' para llegar a la UEFI Shell. ===\n
echo === Cuando aparezca la Shell, ejecuta aplicacion.efi en QEMU y luego Ctrl+C aqui. ===\n\n

continue

# --------------------------------------------------------------------------
# Pasos manuales después del Ctrl+C:
#
# 1. Cambiar arquitectura a 64 bits:
#    (gdb) set architecture i386:x86-64
#
# 2. Encontrar la dirección base donde UEFI cargó la imagen.
#    Opción A — desde la UEFI Shell (antes de ejecutar la app):
#      FS0:\> loadedimage
#    Opción B — buscar el bucle de espera en memoria (si la app ya está corriendo):
#      (gdb) find /b 0x0, 0xffffffff, 0xeb, 0xfe
#      (el compilador emite "jmp -2" = 0xeb 0xfe para un bucle infinito)
#
# 3. Calcular la dirección del .text (el offset es 0x3000 en el .so):
#    (gdb) add-symbol-file aplicacion_debug.so (IMAGE_BASE + 0x3000)
#    Ejemplo si IMAGE_BASE = 0x6500000:
#    (gdb) add-symbol-file aplicacion_debug.so 0x6503000
#
# 4. Salir del bucle de espera:
#    (gdb) set var waiting=0
#
# 5. Poner breakpoint en la instrucción CMP AL, 0xCC:
#    (gdb) break efi_main
#    (gdb) continue
#    -- cuando pare en efi_main, avanzar con nexti hasta el CMP --
#    (gdb) x/20i $rip
#
# 6. Inspeccionar el byte 0xCC en memoria:
#    (gdb) x/1bx &code
#
# 7. Ver registros relevantes:
#    (gdb) info registers rax rbp rsp rip
# --------------------------------------------------------------------------
