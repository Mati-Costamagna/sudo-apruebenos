# TP4: Depuración Dinámica de la Aplicación UEFI con GDB + QEMU

**Objetivo:** Conectar GDB a la aplicación UEFI corriendo dentro de QEMU para inspeccionar la ejecución en vivo: registros, memoria y la instrucción `CMP AL, 0xCC` que valida el byte de breakpoint.

---

## Estructura del directorio

```
parte4/
├── aplicacion.c          # Código fuente con bucle de espera opcional (#ifdef DEBUG_WAIT)
├── Makefile              # Targets normales + targets de debug (run-debug, text-offset)
├── debug.gdb             # Script GDB: conecta a QEMU y documenta los pasos manuales
├── assets/               # Capturas de pantalla del proceso de depuración
└── README.md             # Este archivo
```

---

## 4.1 Modificaciones al código fuente

La única diferencia respecto a la `parte2` es la adición de un bucle de espera condicional al inicio de `efi_main`, compilado únicamente cuando se define `DEBUG_WAIT`:

```c
#ifdef DEBUG_WAIT
    volatile int waiting = 1;
    while (waiting) {}   // GDB conecta aquí y ejecuta: set var waiting=0
#endif
```

**Por qué `volatile`:** sin `volatile`, el compilador puede demostrar que `waiting` nunca cambia dentro del bucle y optimizarlo a un salto infinito `jmp -2` (opcode `eb fe`). Con `volatile`, fuerza una carga desde memoria en cada iteración, lo que hace que GDB pueda modificar `waiting` en el stack frame y romper el bucle.

El compilador con `-O0` genera el siguiente código para el bucle:

```
20fa:  c7 45 fc 01 00 00 00   movl   $0x1,-0x4(%rbp)   # waiting = 1
2101:  90                      nop
2102:  8b 45 fc                mov    -0x4(%rbp),%eax   # eax = waiting
2105:  85 c0                   test   %eax,%eax          # ZF si waiting == 0
2107:  75 f9                   jne    0x2102             # loop si waiting != 0
```

---

## 4.2 Compilación con símbolos de debug

```bash
# Build debug: activa -g, -O0 y -DDEBUG_WAIT
make aplicacion_debug.efi

# Verificar el offset de la sección .text (necesario para GDB)
make text-offset
```

El target `text-offset` muestra la dirección virtual base de `.text` dentro del ELF intermedio:

```
Offset de la seccion .text (usar en add-symbol-file: IMAGE_BASE + offset):
  0x0000000000002000
```

Este valor es fijo para el linker script `elf_x86_64_efi.lds` con `-O0 -g`. Al cargar la imagen en UEFI, la dirección real de `.text` será:

```
DIRECCIÓN_REAL_TEXT = IMAGE_BASE + 0x2000
```

### Layout de efi_main en el binario debug

```
Offset en .so   Instrucción / Propósito
─────────────   ──────────────────────────────────────────────────────────────
0x20d3          efi_main: prólogo (endbr64, push rbp, mov rbp,rsp)
0x20f5          call InitializeLib
0x20fa          movl $0x1, -0x4(%rbp)   ← waiting = 1
0x2102          inicio del bucle de espera (DEBUG_WAIT)
0x2107          jne 0x2102              ← sigue esperando si waiting != 0
0x211d          movb $0xcc, -0x5(%rbp)  ← code[0] = 0xCC
0x2121          movzbl -0x5(%rbp), %eax ← carga code[0] en AL
0x2125          cmp $0xcc, %al          ← ★ INSTRUCCIÓN OBJETIVO ★ (bytes: 3c cc)
0x2127          jne 0x213d              ← salta si code[0] != 0xCC
```

---

## 4.3 Depuración con GDB + QEMU

### Terminal 1: iniciar QEMU con servidor GDB

```bash
make run-debug
```

Equivale a:

```bash
qemu-system-x86_64 \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/ovmf/OVMF.fd \
  -drive format=raw,file=fat:rw:$HOME/UEFI_disk \
  -net none \
  -display gtk \
  -s \     # abre el servidor GDB en localhost:1234 (shorthand de -gdb tcp::1234)
  -S       # pausa la CPU al inicio; espera a que GDB envíe 'continue'
```

QEMU queda con la CPU pausada esperando una conexión GDB.

### Terminal 2: conectar GDB

```bash
gdb -x debug.gdb
```

El script automatiza la conexión y el primer `continue` para llegar a la UEFI Shell.

También se puede hacer paso a paso:

```gdb
(gdb) target remote localhost:1234
(gdb) set architecture i8086          # el firmware arranca en modo real 16-bit
(gdb) continue                        # dejar que OVMF inicialice
```

### Ejecutar la aplicación en QEMU

Con QEMU mostrando la UEFI Shell:

```
FS0:\> aplicacion.efi
```

La app llega al bucle `while (waiting) {}` y se cuelga. La pantalla muestra solo el mensaje de `InitializeLib` (sin avanzar).

### Pausar y cambiar arquitectura

```gdb
^C                                    # Ctrl+C en la terminal de GDB
(gdb) set architecture i386:x86-64   # ahora corremos en modo 64-bit
```

### Encontrar la dirección base de la imagen

UEFI carga el `.efi` en una dirección dinámica asignada por `AllocatePages()`. Hay dos formas de encontrarla:

**Opción A — buscar el patrón del bucle en memoria:**

Con `-O0`, el bucle `while (waiting)` compila a una secuencia identificable. Buscar `movl $0x1` seguido del bucle:

```gdb
(gdb) find /b 0x0, 0x80000000, 0xc7, 0x45, 0xfc, 0x01, 0x00, 0x00, 0x00
```

Si devuelve, por ejemplo, `0x65020fa`, entonces:

```
IMAGE_BASE = 0x65020fa - 0x20fa = 0x6500000
```

**Opción B — leer desde la UEFI Shell antes de ejecutar:**

```
FS0:\> loadedimage
```

Muestra las imágenes cargadas con sus bases. El campo `Image Base` es `IMAGE_BASE`.

### Cargar los símbolos en GDB

Con `IMAGE_BASE` conocido (ejemplo: `0x6500000`):

```gdb
(gdb) add-symbol-file aplicacion_debug.so 0x6502000
#                                          ↑
#                           IMAGE_BASE (0x6500000) + offset .text (0x2000)
```

### Romper el bucle de espera

```gdb
(gdb) set var waiting=0
(gdb) continue
```

La aplicación retoma la ejecución desde donde se quedó.

### Breakpoint en CMP AL, 0xCC

Una vez cargados los símbolos:

```gdb
(gdb) break *0x6502125
#             ↑ IMAGE_BASE + offset de CMP (0x2125)
# O por dirección simbólica si GDB resolvió los símbolos:
(gdb) break efi_main
(gdb) continue
```

Cuando la ejecución para en `CMP AL, 0xCC`:

```gdb
# Ver el byte 0xCC en memoria (variable code en el stack)
(gdb) x/1bx $rbp-0x5
# Salida esperada:
# 0x....: 0xcc

# Ver el estado de los registros relevantes
(gdb) info registers rax rbp rsp rip
# AL (byte bajo de RAX) debe mostrar 0xcc

# Ver las instrucciones alrededor del RIP
(gdb) x/10i $rip
# Debe mostrar:
# => 0x6502125:  cmp    $0xcc,%al
#    0x6502127:  jne    0x650213d
```

---

## 4.4 Análisis del estado en el breakpoint

En el momento en que GDB para en `CMP AL, 0xCC`:

| Registro | Valor esperado | Origen |
|----------|---------------|--------|
| `AL` | `0xcc` | Cargado por `MOVZBL -0x5(%rbp), %eax` en `0x2121` |
| `RBP-0x5` | `0xcc` | Escrito por `MOVB $0xcc, -0x5(%rbp)` en `0x211d` |
| `RIP` | `IMAGE_BASE + 0x2125` | Apunta a la instrucción `CMP` |
| `ZF` (flag) | `0` (aún no ejecutó CMP) | Se actualizará al ejecutar `stepi` |

Después de `stepi` (ejecutar el `CMP`):
- `ZF = 1`: indica que `AL == 0xCC` (condición verdadera)
- El `JNE` siguiente **no se tomará** → la ejecución continúa al `Print` del breakpoint validado

---

## 4.5 Conexión de Ghidra al debugger de QEMU

Ghidra 10+ incluye un debugger integrado que puede conectarse al servidor GDB de QEMU directamente.

### Configuración

1. Abrir el proyecto con `aplicacion.efi` ya importado y analizado (ver Parte 2).
2. Menú: **Debugger → Debug aplicacion.efi**.
3. En el diálogo, seleccionar **"Remote GDB"**.
4. Configurar:
   - **Host:** `localhost`
   - **Port:** `1234`
5. Click en **Connect**.

### Lo que permite el debugger de Ghidra

Una vez conectado, Ghidra sincroniza el análisis estático con la ejecución en vivo:

- El cursor en el **Listing** sigue el `RIP` en tiempo real.
- Click en el margen izquierdo del Listing coloca breakpoints directamente sobre el desensamblado.
- Panel **Registers**: todos los registros actualizados en vivo.
- Panel **Memory**: inspección de cualquier dirección de memoria.
- Panel **Stack**: stack frame actual.
- El panel **Decompiler** (derecha) muestra en qué línea de pseudocódigo C está el `RIP`.

### Flujo recomendado con Ghidra

1. Iniciar QEMU con `make run-debug` → CPU pausada.
2. Abrir Ghidra con `aplicacion.efi` importado.
3. Conectar Ghidra al GDB de QEMU (puerto 1234).
4. En la UEFI Shell, ejecutar `aplicacion.efi` → queda en el bucle de espera.
5. En Ghidra, usar **Ctrl+C** (Suspend) para pausar.
6. Navegar al offset `0x2125` en el Listing → poner breakpoint en `CMP AL, 0xCC`.
7. En Ghidra, **Step Over** hasta liberar el bucle de espera o modificar `waiting` desde el panel **Registers**.
8. **Continue** → la ejecución para justo en `CMP AL, 0xCC`.
9. El panel **Registers** muestra `AL = 0xCC` en vivo.
10. El panel **Decompiler** muestra el cursor parado en `if (local_9 == '\xcc')`.

### Diferencia clave: GDB solo vs Ghidra

| | GDB solo | Ghidra Debugger |
|---|---|---|
| Navegación | Comandos de texto | Visual, click en listing |
| Contexto C | No (solo ASM) | Pseudocódigo sincronizado |
| Breakpoints | Por dirección/función | Click en margen del Listing |
| Memoria | `x/...` manual | Panel interactivo |
| Ideal para | Automatización, scripting | Análisis estático + dinámico combinado |

---

## 4.6 Comandos de referencia rápida

```bash
# Compilar versión debug
make aplicacion_debug.efi

# Verificar offset de .text
make text-offset

# Lanzar QEMU con GDB server
make run-debug

# Conectar GDB con el script automático
gdb -x debug.gdb
```

```gdb
# Conexión y arquitectura
target remote localhost:1234
set architecture i8086
continue
# [esperar UEFI Shell, ejecutar app, Ctrl+C]
set architecture i386:x86-64

# Cargar símbolos (ajustar IMAGE_BASE)
add-symbol-file aplicacion_debug.so (IMAGE_BASE + 0x2000)

# Liberar bucle de espera
set var waiting=0

# Breakpoint en CMP AL, 0xCC
break *IMAGE_BASE+0x2125
continue

# Inspección en el breakpoint
x/1bx $rbp-0x5          # debe mostrar 0xcc
info registers rax rip   # AL = 0xcc, RIP apunta al CMP
x/5i $rip                # listing del punto de parada
```
