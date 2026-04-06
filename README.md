# Índice GINI y Convención de Llamadas

**Asignatura:** Sistemas de Computación  

**Profesores:** 
  - Jorge, Javier Alejandro
  - Solinas, Miguel Angel

**Estudiantes:** 
  - Costamagna, Matias
  - Davila Tomassi, Carlos Valentino
  - Sabena, Maria Pilar

**Link del repositorio:** https://github.com/Mati-Costamagna/sudo-apruebenos/tree/stack_frame

**Fecha:** Abril 2026

---

La pregunta que guió todo el trabajo fue simple: ¿cómo hace Python para hablarle a una función escrita en Assembly? No es magia — hay una cadena de convenciones muy específicas que este TP intenta desnudar capa por capa.

El dominio elegido fue el Índice GINI del Banco Mundial, no porque sea especialmente interesante como dato, sino porque tiene la forma correcta: un número flotante que viaja desde una API hasta una instrucción de CPU, pasando por tres lenguajes distintos.

---

## Objetivos

El objetivo general fue implementar una calculadora de índices GINI integrando Python, C y Assembly para entender de primera mano cómo los lenguajes de alto nivel terminan mapeándose a instrucciones concretas de CPU.

Para llegar ahí, nos propusimos:
1. Consumir datos reales desde la API REST del Banco Mundial
2. Integrar C desde Python usando ctypes (FFI)
3. Implementar un Stack Frame genuino en x86-64, forzando argumentos al stack
4. Dominar la convención System V AMD64 ABI
5. Acceder a parámetros via desplazamientos relativos a `%rbp`
6. Debuggear con GDB inspeccionando el stack en tiempo real
7. Comparar la performance entre las tres implementaciones (con resultados que sorprenden)

---

## Arquitectura del Sistema

```
┌─────────────────────────────────────────────────────────────┐
│                    Python (Alto Nivel)                      │
│    • Requests a API Banco Mundial                           │
│    • Parseo JSON + Business Logic                           │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ↓ (ctypes FFI)
┌─────────────────────────────────────────────────────────────┐
│                  C (Nivel Intermedio)                       │
│    • Interfaz limpia para Python                            │
│    • Wrapper que agrega Dummy Arguments                     │
│    • Llamadas a funciones en Assembly                       │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ↓ (System V AMD64 ABI)
┌─────────────────────────────────────────────────────────────┐
│              Assembly x86-64 (Bajo Nivel)                   │
│    • Acceso directo a Stack Frame                           │
│    • Manipulación de registros (%rax, %xmm0, etc)           │
│    • Instrucciones de CPU (cvttss2si, etc)                  │
└─────────────────────────────────────────────────────────────┘
```

---

## Fase 1: Integración Python + C

La primera fase establece la comunicación entre Python y una librería compartida en C mediante ctypes, sin todavía bajar al Assembly. El objetivo era entender el "puente" antes de complicar la lógica del otro lado.

### Flujo de Datos
![Diagrama de Flujo Fase 1](fase1/diagrama_fase1.svg)

Python llama a ctypes, ctypes se encarga del marshalling de tipos (convierte el `float` de Python a un `c_float` que C puede recibir), y C hace la operación:

```python
# fase1/main.py
import requests
import ctypes

# 1. Obtener datos de API
response = requests.get(WORLDBANK_API_URL)
data = response.json()

# 2. Cargar librería C
lib = ctypes.CDLL('./libgini.so')
lib.float_to_int.argtypes = [ctypes.c_float]
lib.float_to_int.restype = ctypes.c_int

# 3. Llamar C desde Python
gini_int = lib.float_to_int(42.3)  # ← ctypes marshals: float → c_float
```

```c
// fase1/gini.c - C Puro (SIN Assembly aún)
int float_to_int(float value) {
    return (int)value;  // Conversión simple en C
}

int sumar_uno(int value) {
    return value + 1;
}
```

### Ejecución Fase 1
```bash
$ cd fase1
$ make all
$ python3 main.py

Resultados: Argentina (Últimos 5 años) - FASE 1 (C Pura)
AÑO    | GINI ORIGINAL   | ENTERO (C) | SUMA +1 (C)
-------+-------+--------+-------+--------+----------
2020   | 42.30           | 42        | 43
2019   | 41.50           | 41        | 42
...
```

---

## Fase 2: Stack Frame y Convención de Llamadas {#fase-2}

Acá empieza lo interesante. La segunda fase reemplaza la lógica de C por rutinas en Assembly x86-64, con el foco puesto en entender y manipular el Stack Frame. No alcanzaba con leerlo en la teoría — había que hacer que un valor llegara al stack y luego leerlo desde Assembly para que tuviera sentido.

### Flujo de Datos
![Diagrama de Flujo Fase 2](fase2/diagrama_flujo.svg)

### Diagrama de Secuencia
![Diagrama de Secuencia Fase 2](fase2/diagrama_secuencia.svg)

### La Estrategia: Dummy Arguments

El problema es que la convención System V AMD64 ABI pasa los primeros argumentos en registros, no en el stack. Si pasamos un solo `float`, va directo a `%xmm0` y nunca toca el stack. Para forzar el comportamiento que queremos estudiar, usamos una técnica pedagógica: saturar todos los registros disponibles con argumentos basura antes de pasar el valor real.

**System V AMD64 ABI define:**
- **6 registros enteros**: `%rdi`, `%rsi`, `%rdx`, `%rcx`, `%r8`, `%r9`
- **8 registros XMM** (punto flotante): `%xmm0`–`%xmm7`

Eso son 14 registros. El argumento número 15 no tiene a dónde ir — debe ir al stack.

```c
// fase2/gini.c
int float_to_int(float value) {
    // Pasamos 15 argumentos a propósito:
    return asm_float_to_int(
        0,0,0,0,0,0,          // 6 enteros → %rdi, %rsi, %rdx, %rcx, %r8, %r9
        0.0,0.0,0.0,0.0,      // 8 doubles → %xmm0-%xmm7
        0.0,0.0,0.0,0.0,
        value                 // 15º argumento → STACK
    );
}
```

Desde Assembly, el valor ya está en el stack y podemos leerlo con un offset relativo a `%rbp`:

```asm
# fase2/gini_asm.s
asm_float_to_int:
    pushq   %rbp           # Guardar RBP anterior
    movq    %rsp, %rbp     # Crear nuevo frame

    movss   0x10(%rbp), %xmm0   # ← Leer float desde stack
    cvttss2si %xmm0, %eax       # Convertir float→int
    
    popq    %rbp
    ret
```

El `0x10` no es arbitrario — tiene una razón exacta que explicamos en la sección siguiente.

### Ejecución Fase 2
```bash
$ cd fase2
$ make all
$ python3 main.py

Resultados para: Argentina (Últimos 5 años) - FLUJO COMPLETO (Python → C → ASM)
AÑO    | GINI ORIGINAL   | ENTERO (C/ASM) | SUMA +1 (C/ASM)
-------+-------+--------+-------+--------+----------
2020   | 42.30           | 42             | 43
2019   | 41.50           | 41             | 42
...
```

---

## Stack Frame Layout (Análisis Detallado)

### Una aclaración importante sobre las arquitecturas

En la clase teórica se trabaja con **x86 de 32 bits** y el registro **EBP**. Este proyecto corre en **x86-64 de 64 bits**, lo que cambia los tamaños de todo:

| Aspecto | x86 (32-bit) | x86-64 (64-bit) |
|---------|--------------|-----------------|
| Registro Base | **EBP** | **RBP** |
| Tamaño al guardar | 4 bytes | 8 bytes |
| Offset 1er parámetro en stack | `EBP + 8` | **`RBP + 16` (0x10)** |
| ISA | IA-32 | AMD64/System V |

La lógica es la misma; solo cambia el word size. Eso es lo que explica el `0x10`.

### Diagrama Interactivo
![Diagrama del Stack Frame](fase2/diagrama_stack.svg)

### ¿Por qué exactamente `0x10(%rbp)`?

Cuando `call` invoca a `asm_float_to_int`, guarda la dirección de retorno en el stack. Después, el prólogo de la función guarda el `%rbp` anterior. Al terminar el prólogo, el stack quedó así:

```
ANTES de pushq %rbp:          DESPUÉS de pushq %rbp:        DESPUÉS de movq %rsp, %rbp:
┌───────────────────┐         ┌───────────────────┐         ┌───────────────────┐
│ gini_value (4B)   │ RSP+8   │ gini_value (4B)   │         │ gini_value (4B)   │
├───────────────────┤         ├───────────────────┤         ├───────────────────┤
│ RIP (8B) ← CALL   │ RSP     │ RIP (8B) ← CALL   │ RSP+8   │ RIP (8B) ← CALL   │ 0x08(%rbp)
│(dirección retorno)|         │(dirección retorno)|         │(dirección retorno)│
├───────────────────┤         ├───────────────────┤         ├───────────────────┤
│       ⬇           │         │ RBP_anterior (8B) │ RSP     │ RBP_anterior (8B) │ 0x00(%rbp)
│      RSP          │         │      ⬇            │         │      ⬇            │ ← %rbp
└───────────────────┘         │      RSP          │         │      RSP          │
                              └───────────────────┘         └───────────────────┘
```

Contando desde `%rbp`:

| Contenido | Tamaño | Offset |
|-----------|--------|--------|
| RBP anterior (guardado por `pushq %rbp`) | 8 bytes | `0x00(%rbp)` |
| RIP — dirección de retorno (guardado por `call`) | 8 bytes | `0x08(%rbp)` |
| **gini_value — nuestro float** | 4 bytes | **`0x10(%rbp)`** ✓ |

8 + 8 = 16 = 0x10. Si fuera x86 de 32 bits, serían 4 + 4 = 8 = 0x08, que es lo que se ve en las diapositivas de clase.

### Instrucciones Clave

```asm
pushq   %rbp                    # RSP -= 8; [RSP] = %rbp anterior
movq    %rsp, %rbp             # %rbp = %rsp (crea nuevo frame)

# Ahora podemos acceder a argumentos del stack:
movss   0x10(%rbp), %xmm0      # Lee float desde dirección (RBP + 0x10)
                               # Equivalente a: memoria[RBP + 16] → XMM0

cvttss2si %xmm0, %eax          # float (XMM0) → int (EAX), truncando

popq    %rbp                    # %rbp = [RSP]; RSP += 8 (restaura)
ret                             # Salta a dirección guardada en [RSP]
```

---

## Casos de Prueba

La instrucción `cvttss2si` (Convert with Truncation) trunca hacia **cero**, no redondea. Eso significa que `42.99` da `42`, y `-15.7` da `-15`. Al principio parece raro, pero es el comportamiento definido: truncar es cortar la parte decimal, no buscar el entero más cercano.

| # | Entrada (float) | Esperado (int) | Actual (int) | Suma +1 |
|---|-----------------|----------------|--------------|---------|
| 1 | 42.3 | 42 | 42 | 43 |
| 2 | 42.99 | 42 | 42 | 43 |
| 3 | 3.14159 | 3 | 3 | 4 |
| 4 | 0.5 | 0 | 0 | 1 |
| 5 | -15.7 | -15 | -15 | -14 |
| 6 | 0.0 | 0 | 0 | 1 |
| 7 | 99.99 | 99 | 99 | 100 |

---

## Análisis de Performance

### Benchmark: Python vs C vs C+Assembly

```bash
$ python3 benchmark.py
```

#### Resultados Típicos (en μs por llamada)

```
Implementación         Tiempo promedio    Speedup relativo
──────────────────────────────────────────────────────────
Python (int())         0.025 μs           1.00x (baseline)
C (Fase 1)             1.200 μs           0.02x (FFI overhead)
C+ASM (Fase 2)         1.210 μs           0.02x (FFI overhead)
```

### Lo que muestran los números

El resultado más contraintuitivo del TP: **Python ganó**. No porque `int()` sea una maravilla de optimización, sino porque el overhead fijo de ctypes (~0.8–1.0 μs por llamada) eclipsa completamente cualquier ventaja que tenga C o Assembly cuando la operación en sí es trivial. C y C+ASM son prácticamente indistinguibles por la misma razón.

Esto no significa que Assembly no sirva. Significa que hay que elegir bien dónde usarlo:
- Cuando la lógica es **compleja** (loops, vectorización SIMD, etc.) el overhead de FFI pasa a ser una fracción pequeña del tiempo total
- Cuando la operación tarda **menos de 10 μs**, el overhead domina y no hay mucho por ganar
- Cuando tarda **más de 100 μs**, el overhead es menor al 1% y Assembly puede marcar diferencia real

La lección más importante: **medir antes de asumir**. Los supuestos sobre performance sin datos frecuentemente están equivocados.

---

## Debugging con GDB

### Preparación

```bash
$ cd fase2
$ make clean
$ make DEBUG=1  # Compilar con símbolos de debug
```

### Verificando el Stack Frame en Vivo

La parte más útil del TP fue poder pausar la ejecución dentro de `asm_float_to_int` y ver exactamente lo que predijimos en papel:

```gdb
(gdb) break asm_float_to_int
(gdb) run
(gdb) print $rbp
$1 = (void *) 0x7fffffffe260

(gdb) x/5gx $rbp
0x7fffffffe260:	0x00007fffffffe280	0x0000555555554a4d
0x7fffffffe270:	0x0000000000000000	0x41267e2d00000000
0x7fffffffe280:	0x00007fffffffe2c0	0x0000555555554b15
```

- `[RBP+0x00]` = `0x00007fffffffe280` → RBP anterior ✓
- `[RBP+0x08]` = `0x0000555555554a4d` → Dirección de retorno ✓
- `[RBP+0x10]` = `0x41267e2d` → **nuestro float 42.3 en IEEE 754** ✓

Para confirmar el valor:

```gdb
(gdb) print *(float*)($rbp + 0x10)
$2 = 42.2999992...

(gdb) print (int)42.2999992
$3 = 42
```

Todo coincide con el análisis teórico.

```gdb
(gdb) info frame
Stack level 0, frame at 0x7fffffffe260:
 rip = 0x555555554a2a in asm_float_to_int (gini_asm.s:23)
 rsp = 0x7fffffffe250
 rbp = 0x7fffffffe260
 saved rbp 0x7fffffffe280
 saved rip 0x555555554a4d
 called by frame at 0x7fffffffe280
 source language asm
 Arglist at 0x7fffffffe260, args: [locals 0, registers available 0]
 Locals at 0x7fffffffe260, Previous frame's sp is 0x7fffffffe270
 Saved registers:
  rbp at 0x7fffffffe260
  rip at 0x7fffffffe268
```

![Inspección del Stack](fase2/assets/gdb_screenshot.png)

![Info Frame](fase2/assets/gdb_info_frame.png)

---

## Resultados Finales

```bash
$ cd fase2
$ python3 main.py
```

![Resultado Ejecución](fase2/assets/output.png)

---

## Conclusiones

Después de implementar el sistema completo, tres cosas quedaron claras:

**El Stack Frame deja de ser abstracto cuando lo ves en GDB.** Leer en la teoría que el primer parámetro está en `EBP+8` (o `RBP+0x10` en 64 bits) es una cosa. Pausar la ejecución dentro de `asm_float_to_int`, escribir `x/5gx $rbp` en GDB y ver el valor `0x41267e2d` exactamente donde predijiste que iba a estar es otra completamente distinta.

**La convención de llamadas no es un detalle opcional.** Trabajar con tres lenguajes al mismo tiempo obliga a respetar el contrato de System V AMD64 ABI de forma estricta: qué va en cada registro, en qué orden, qué hay que preservar. Un byte en el lugar equivocado y el programa falla de formas difíciles de diagnosticar.

**El benchmark arruinó los supuestos iniciales, y eso estuvo bien.** Antes de medir, asumíamos que C+ASM iba a ser notablemente más rápido que Python. El resultado fue el opuesto. El overhead fijo de ctypes es real y significativo para operaciones simples. Assembly tiene su lugar, pero ese lugar no es cualquier lugar — es donde la lógica es lo suficientemente compleja como para que el costo de cruzar la frontera valga la pena. La técnica de los dummy arguments en particular no es algo que usaríamos en producción; es una herramienta pedagógica para demostrar que el stack existe y que podemos controlarlo.
