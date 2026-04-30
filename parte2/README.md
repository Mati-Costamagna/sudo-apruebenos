# TP2: Desarrollo, Compilación y Análisis de Seguridad UEFI

**Objetivo:** Crear una aplicación nativa UEFI en C, compilarla al formato binario PE/COFF y analizar cómo un descompilador interpreta sus opcodes a nivel de firmware.

---

## Estructura del directorio

```
parte2/
├── aplicacion.c    # Código fuente de la aplicación UEFI
├── Makefile        # Automatización del proceso de compilación en 3 etapas
└── README.md       # Este archivo
```

---

## 2.1 Desarrollo de la Aplicación

![Fases de arranque PI](assets/pi_boot_phases.svg)

El diagrama muestra las 5 fases de Platform Initialization (PI). Las fases resaltadas en verde (DXE) y naranja (BDS) son las relevantes para este trabajo: DXE establece el entorno de 64 bits con GDT y servicios UEFI, y BDS es quien lee `BootOrder` de NVRAM, llama a `LoadImage()` y finalmente ejecuta `aplicacion.efi`.

El archivo `aplicacion.c` implementa una aplicación UEFI mínima. A diferencia de un programa C estándar, el punto de entrada no es `main()` sino `efi_main()`, que recibe:

- `EFI_HANDLE ImageHandle`: identificador de la imagen cargada en memoria.
- `EFI_SYSTEM_TABLE *SystemTable`: puntero a la tabla del sistema, que expone Boot Services, Runtime Services y la consola de salida (`ConOut`).

La aplicación:
1. Inicializa la biblioteca `gnu-efi` con `InitializeLib()`.
2. Imprime un mensaje en pantalla usando `SystemTable->ConOut->OutputString`.
3. Define un array de un byte con el valor `0xCC` (opcode x86 de la instrucción `INT3`).
4. Verifica en tiempo de ejecución si ese byte es `0xCC` e imprime el resultado.

### ¿Por qué `OutputString` en lugar de `printf`?

> **Pregunta de Razonamiento 4**

En el entorno pre-OS de UEFI no existe una biblioteca estándar de C (`libc`). Funciones como `printf` dependen de llamadas al sistema operativo (syscalls) que todavía no existen. `SystemTable->ConOut->OutputString` es la interfaz provista por el propio firmware para escribir en consola, y opera directamente sobre el hardware de video sin necesidad de un OS.

---

## 2.2 Compilación a Formato PE/COFF

UEFI utiliza el formato **PE/COFF** (Portable Executable / Common Object File Format), el mismo formato de los `.exe` de Windows. Esto aplica incluso cuando compilamos desde Linux.

> El formato más primitivo de ejecutable bare-metal es el **MBR**, estudiado en el trabajo anterior: 512 bytes crudos, sin encabezado, cargado siempre en la dirección física fija `0x7C00`. PE/COFF es la respuesta moderna a ese diseño — agrega un encabezado estructurado, secciones nombradas y una tabla de reubicaciones (`.reloc`) que permite cargarlo en cualquier dirección física disponible.

El proceso tiene **3 etapas**:

### Etapa 1 — Compilar a código objeto

```bash
gcc -I/usr/include/efi -I/usr/include/efi/x86_64 -I/usr/include/efi/protocol \
    -fpic -ffreestanding -fno-stack-protector -fno-strict-aliasing \
    -fshort-wchar -mno-red-zone -maccumulate-outgoing-args \
    -Wall -c -o aplicacion.o aplicacion.c
```

Flags clave:
- `-ffreestanding`: indica al compilador que no hay biblioteca estándar de C.
- `-fpic`: genera código independiente de posición (necesario para reubicación).
- `-fshort-wchar`: UEFI usa `wchar_t` de 2 bytes (UTF-16), no 4.
- `-mno-red-zone`: deshabilita la red zone de x86-64, que el firmware no puede garantizar.

### Etapa 2 — Enlazar a objeto compartido intermedio

```bash
ld -shared -Bsymbolic \
   -L/usr/lib -L/usr/lib/efi \
   -T /usr/lib/elf_x86_64_efi.lds \
   /usr/lib/crt0-efi-x86_64.o \
   aplicacion.o -o aplicacion.so -lefi -lgnuefi
```

Se usa el linker script `elf_x86_64_efi.lds` que organiza las secciones de memoria en el orden que espera UEFI. `crt0-efi-x86_64.o` es el stub de arranque que llama a `efi_main`.

### Etapa 3 — Convertir a ejecutable EFI (PE/COFF)

```bash
objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym \
        -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc \
        --target=efi-app-x86_64 aplicacion.so aplicacion.efi
```

`objcopy` extrae solo las secciones relevantes del ELF y las reempaqueta en formato PE/COFF con el subsistema `EFI_APPLICATION`.

### Compilar con Make

```bash
# Instalar dependencias (si no están) y compilar
sudo apt install gnu-efi build-essential && make
```

---

## 2.3 Análisis de Metadatos y Decompilación

### Verificar el tipo de binario

```bash
file aplicacion.efi
```

Salida esperada:
```
aplicacion.efi: MS-DOS executable PE32+ executable (EFI application) x86-64, for MS Windows
```

El sistema reconoce el formato PE/COFF. Aunque fue compilado en Linux para UEFI, `file` lo identifica como un ejecutable Windows porque ambos comparten el mismo formato binario.

### Inspeccionar el encabezado

Una vez que `objcopy` convierte el binario a PE/COFF, **ya no es un archivo ELF** — tiene el magic number `MZ` de MS-DOS en lugar de `\x7fELF`. Por eso `readelf` falla sobre el `.efi`.

```bash
# Cabecera PE/COFF del ejecutable final
objdump -f aplicacion.efi
objdump -p aplicacion.efi

# Cabecera ELF del objeto intermedio (antes de la conversión)
readelf -h aplicacion.so
```

#### Análisis de la salida de `objdump -f`

```
aplicacion.efi:     file format pei-x86-64
architecture: i386:x86-64, flags 0x00000133:
HAS_RELOC, EXEC_P, HAS_SYMS, HAS_LOCALS, D_PAGED
start address 0x0000000000002000
```

| Campo | Valor | Significado |
|-------|-------|-------------|
| `file format` | `pei-x86-64` | Formato PE/COFF para x86-64 (el "i" indica imagen ejecutable, no objeto relocatable) |
| `HAS_RELOC` | presente | El binario contiene una tabla `.reloc` con fixups de reubicación. Esto es lo que permite a `LoadImage()` cargarlo en cualquier dirección física disponible, a diferencia del BIOS que ejecutaba código desde una dirección fija |
| `EXEC_P` | presente | Es una imagen ejecutable (tiene punto de entrada definido) |
| `start address` | `0x2000` | Dirección virtual relativa (RVA) del punto de entrada. No es una dirección física absoluta — el firmware sumará la base real en tiempo de carga |

#### Análisis de la salida de `objdump -p`

```
Magic     020b   (PE32+)
Subsystem 0000000a   (EFI application)
ImageBase 0000000000000000
AddressOfEntryPoint  0000000000002000
Entry 5   0000000000003000 0000000c   Base Relocation Directory [.reloc]
```

| Campo | Valor | Significado |
|-------|-------|-------------|
| `Magic 020b` | PE32+ | Indica ejecutable de 64 bits (PE32 sería `010b` para 32 bits). UEFI en x86-64 requiere PE32+ |
| `Subsystem 0x000a` | EFI application | El firmware identifica este campo para saber qué tipo de imagen cargar. `0x0a` = aplicación UEFI (no driver, no runtime service) |
| `ImageBase 0x0` | cero | La base de carga preferida es cero intencionalmente: el firmware asignará la dirección real mediante `AllocatePages()` y aplicará los fixups de `.reloc`. Es el mecanismo de **ASLR del pre-OS**. Contrasta directamente con el MBR del trabajo anterior, que no tiene reubicación posible: el BIOS siempre lo carga en `0x7C00` y el código asume esa dirección de forma rígida |
| `.reloc` en Entry 5 | `0x3000`, tamaño `0xc` | Sección de reubicaciones con 2 fixups. Sin esta sección, el firmware no podría reubicar el binario y la carga fallaría |
| `SizeOfCode` | `0x200` (512 B) | El código compilado de nuestra aplicación ocupa solo 512 bytes, lo que ilustra la naturaleza minimalista de las aplicaciones UEFI |

#### Análisis de la salida de `readelf -h` (sobre `aplicacion.so`)

```
Magic:   7f 45 4c 46 02 01 01 00 ...
Type:    DYN (Shared object file)
Entry point address: 0x2000
Number of section headers: 22
```

| Campo | Valor | Significado |
|-------|-------|-------------|
| `Magic \x7fELF` | ELF | El objeto intermedio `.so` es un ELF válido. Al ejecutar `objcopy`, este magic cambia a `MZ` (0x4D5A), convirtiendo el archivo en PE/COFF |
| `Type DYN` | Shared object | El linker produce un objeto compartido (no un ejecutable ELF estándar) porque UEFI requiere código reubicable con tabla de símbolos dinámica, que luego `objcopy` transforma al formato PE |
| `Entry point 0x2000` | mismo en ambos | La RVA del punto de entrada se preserva idéntica entre el ELF intermedio y el PE final, lo que confirma que `objcopy` no reordena el código sino que reempaqueta los metadatos |
| `22 section headers` | vs 6 en el .efi | El ELF tiene 22 secciones (debug, dynamic linking, etc.). `objcopy` extrae solo las 6 necesarias para UEFI (`.text`, `.data`, `.reloc`, etc.), descartando todo lo demás |

### Decompilación con Ghidra

#### ¿Qué es Ghidra?

Ghidra es un framework de ingeniería inversa desarrollado por la **NSA (National Security Agency)** y publicado como software libre en 2019. Su función principal es analizar binarios compilados sin acceso al código fuente, reconstruyendo información de alto nivel a partir de los bytes del ejecutable.

Para lograrlo, Ghidra ejecuta varias etapas de análisis sobre el binario:

1. **Carga y parseo del formato:** identifica el tipo de archivo (PE/COFF, ELF, Mach-O, etc.), extrae las secciones, el punto de entrada y la tabla de símbolos si existe.

2. **Desensamblado (Disassembly):** recorre los bytes del segmento `.text` e interpreta cada secuencia como una instrucción del conjunto de instrucciones del procesador (en nuestro caso x86-64). El resultado es el panel **Listing**, que muestra dirección, bytes crudos e instrucción en ensamblador. En el trabajo anterior usamos `objdump -S` para hacer exactamente lo mismo sobre el objeto ensamblado del MBR — Ghidra automatiza ese proceso y lo extiende con análisis de flujo.

3. **Análisis de flujo de control:** construye el grafo de flujo de control (CFG) identificando saltos, llamadas y retornos. Esto le permite delimitar funciones automáticamente aunque no haya símbolos de debug.

4. **Análisis de tipos y datos:** infiere tipos de variables locales observando cómo se usan los registros y el stack. Por ejemplo, si un valor se pasa a `OutputString` como segundo argumento, infiere que es un puntero a string.

5. **Decompilación:** a partir del CFG y los tipos inferidos, el motor decompilador reconstruye pseudocódigo C en el panel **Decompiler**. Este pseudocódigo no es el código fuente original sino una aproximación semánticamente equivalente, escrita por Ghidra para hacer el análisis más legible.

En el contexto de seguridad de firmware, Ghidra es especialmente relevante porque puede analizar imágenes PE/COFF (el formato de UEFI) y reconstruir la lógica de drivers o aplicaciones EFI sin necesidad de su código fuente, permitiendo detectar backdoors, bootkits o código malicioso embebido en el firmware.

#### Limitaciones del decompilador

Ghidra realiza su propio análisis de flujo de datos independientemente del compilador. Si puede determinar que una condición es siempre verdadera o falsa (por ejemplo, que una variable en el stack siempre vale `0xCC` porque fue escrita con ese valor y nunca modificada), elimina el branch en el pseudocódigo aunque la instrucción `CMP` y `JNZ` existan en el ensamblado. Esto puede ocultar lógica que sí está presente en el binario — razón por la cual siempre es necesario contrastar el panel **Decompiler** con el panel **Listing**.

Para ver la comparación contra `0xCC` como valor con signo (`-52`) directamente en el Listing: click derecho sobre el operando `0xcc` en la instrucción `CMP AL, 0xcc` → **Convert → Signed Decimal**.

#### Instalación

Ghidra no tiene paquete `apt` ni `snap`. Requiere Java 21+ y descarga manual.

```bash
# 1. Instalar Java (si no está)
sudo apt install openjdk-21-jdk

# 2. Descargar la última versión
wget https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_12.0.4_build/ghidra_12.0.4_PUBLIC_20260303.zip -O ghidra.zip

# 3. Extraer en /opt
sudo unzip ghidra.zip -d /opt/
sudo mv /opt/ghidra_12.0.4_PUBLIC /opt/ghidra

# 4. Crear symlink para ejecutar desde cualquier lugar
sudo ln -s /opt/ghidra/ghidraRun /usr/local/bin/ghidra

# 5. Lanzar
ghidra
```

1. Crear un nuevo proyecto y seleccionar **Import File** → `aplicacion.efi`.
2. Ghidra detecta automáticamente el formato PE/COFF y la arquitectura x86-64.
3. Ejecutar el análisis automático.
4. Navegar a la función `efi_main` en el árbol de símbolos.

#### Captura: vista de Ghidra sobre `efi_main`

![Ghidra decompile efi_main](./assets/ghidra_efi_main.png)

**Pasos para llegar a esta vista:**

1. Se compiló `aplicacion.c` a `aplicacion.efi` mediante las 3 etapas del Makefile (`gcc` → `ld` → `objcopy`).
2. Se instaló Ghidra 12.0.4 y se creó un nuevo proyecto (**File → New Project**).
3. Se importó el binario (**File → Import File** → `aplicacion.efi`). Ghidra detectó automáticamente el formato PE/COFF x86-64.
4. Se abrió el binario con doble click y se ejecutó el análisis automático (**Yes → Analyze**).
5. En el **Symbol Tree** (panel izquierdo) → **Functions** → `efi_main`, que abre simultáneamente el panel de desensamblado (izquierda) y el decompilador (derecha).

**Qué muestra la captura:**

- **Panel izquierdo — Listing (desensamblado):** muestra las instrucciones x86-64 en ensamblador. Se ve el inicio de `efi_main` en la dirección `0x000020d3` con la instrucción `ENDBR64` (protección CET), las variables locales en el stack (`local_9` en `Stack[-0x9]`, `local_20`, `local_28`) y sus referencias cruzadas (XREF) a las direcciones donde se leen y escriben.

- **Panel derecho — Decompiler:** muestra el pseudocódigo C reconstruido por Ghidra. Se observa:
  - La firma aparece como `efi_main(void)` porque Ghidra no resuelve automáticamente los tipos UEFI (`EFI_HANDLE`, `EFI_SYSTEM_TABLE`).
  - `unaff_RSI` representa el puntero `SystemTable` pasado por registro, que Ghidra no logra nombrar sin información de tipos UEFI.
  - Las llamadas a `OutputString` se ven como dobles dereferences de puntero: `(**(...)(unaff_RSI + 0x40) + 8))`, que corresponde exactamente a `SystemTable->ConOut->OutputString`.
  - La cadena `u_Breakpoint_estatico_alcanzado` confirma que Ghidra identificó el string Unicode de la aplicación.
  - Notablemente, la condición `if (code[0] == 0xCC)` **no aparece en el pseudocódigo**: el compilador optimizó el branch porque el valor `0xCC` es una constante conocida en tiempo de compilación, por lo que la condición siempre es verdadera y se eliminó el chequeo por ser redundante.

**Sin `-O0` (optimización por defecto):** el compilador evalúa `code[]` en tiempo de compilación. Como `{ 0xCC }` es un literal constante, sabe que `code[0] == 0xCC` siempre es verdadero, elimina el `if` por completo y emite solo el cuerpo del bloque. En Ghidra el branch desaparece — no hay instrucción `CMP` ni salto condicional `JNE` en el desensamblado.

**Con `-O0` (agregado al Makefile):** el compilador desactiva todas las optimizaciones y trata cada variable como si pudiera cambiar. Emite código literal para cada línea del fuente: escribe `0xCC` en el stack, luego hace un `CMP byte ptr [rbp-0x1], 0xCC` y un `JNZ` (salto si no es igual). En Ghidra aparece la condición explícita, lo que permite ver la lógica original y detectar el patrón `INT3` durante el análisis estático.

#### Pregunta de Razonamiento 5

> **¿Por qué `0xCC` aparece como `-52` en el pseudocódigo de Ghidra?**

`0xCC` en hexadecimal equivale a `204` en decimal sin signo. Ghidra, al descompilar, interpreta los bytes como enteros con signo de 8 bits (**signed byte**). En complemento a dos de 8 bits:

```
0xCC = 1100 1100b
     = -(256 - 204)
     = -52
```

![Ghidra CMP AL -52](./assets/ghidra_cmp_signed_52.png)

La captura muestra el bloque central de `efi_main` en el panel Listing con la representación en decimal con signo activada. Los puntos clave:

- **`MOV byte ptr [RBP + local_9], 0xcc` (`0x211d`):** el compilador escribe el valor del array `code[]` en el stack. Con `-O0` esta instrucción existe explícitamente; sin él el compilador la elimina por optimización.
- **`MOVZX EAX, byte ptr [RBP + local_9]` (`0x2121`):** carga el byte del stack en `EAX` con extensión de cero a 32 bits. Ghidra lo usa para inferir que `local_9` es de tipo `byte`.
- **`CMP AL, -52` (`0x2125`):** la instrucción en bytes es `3c cc`. Al aplicar **Convert → Signed Decimal**, Ghidra reescribe el operando inmediato `0xcc` como `-52`. El procesador opera sobre el mismo patrón de bits `11001100` sin importar la interpretación — son los mismos bytes `3c cc` en ambos casos.
- **`JNZ LAB_0000214c` (`0x2127`):** salto condicional que se tomaría si `AL != 0xCC`. En la ejecución real nunca se toma, pero existe en el binario gracias a `-O0`.

**Importancia en ciberseguridad:** Los analistas de malware deben reconocer que `-52` y `0xCC` son el mismo opcode `INT3`. Este byte es usado como:
- **Breakpoint de debugging**: depuradores como GDB lo insertan temporalmente para pausar la ejecución.
- **Anti-debugging**: malware puede escanear su propio código buscando `0xCC` para detectar si un debugger está activo.
- **Shellcode**: en exploits, un `INT3` inesperado puede indicar un payload incompleto o una trampa.

Si un analista no reconoce la equivalencia `-52 ↔ 0xCC`, puede pasar por alto una técnica de evasión o un breakpoint intencionalmente dejado en el código.

---

## Dependencias del sistema

```bash
sudo apt update
sudo apt install -y gnu-efi build-essential binutils
```

| Paquete | Rol |
|---------|-----|
| `gnu-efi` | Headers (`efi.h`, `efilib.h`) y runtime de arranque UEFI |
| `build-essential` | GCC, make y utilidades base |
| `binutils` | `ld` y `objcopy` para el proceso de linkeo y conversión |

---

## gnu-efi vs EDK2/TianoCore

Existen dos toolchains principales para desarrollar aplicaciones UEFI en C. Este trabajo usa **gnu-efi**, pero es importante conocer la alternativa oficial: **EDK2/TianoCore**, que es el framework de referencia utilizado por Intel, AMD y la industria para desarrollar firmware real. El repositorio [UEFI-Lessons](https://github.com/javierbrk/UEFI-Lessons) es una referencia de aprendizaje basada en EDK2 que ilustra estas diferencias.

| | **gnu-efi** (este TP) | **EDK2 / TianoCore** |
|---|---|---|
| Toolchain | `gcc` + `ld` + `objcopy` | Build system propio (`build` + Python + NASM) |
| Configuración | `Makefile` directo | Archivos `.inf` (módulo) + `.dsc` (paquete) |
| Punto de entrada | `efi_main()` | `UefiMain()` con macro `EFIAPI` |
| Headers | `<efi.h>` / `<efilib.h>` | `<Uefi.h>` / `MdePkg` |
| Complejidad | Mínima — ideal para aprendizaje | Alta — adecuada para firmware de producción |
| Uso típico | Apps simples, investigación | Drivers, firmware de plataforma, BIOS comercial |

### Diferencia en el código fuente

**gnu-efi** (nuestro `aplicacion.c`):
```c
#include <efi.h>
#include <efilib.h>

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hola\r\n");
    return EFI_SUCCESS;
}
```

**EDK2** (equivalente de `Lesson_03` de UEFI-Lessons):
```c
#include <Uefi.h>
#include <Library/UefiLib.h>

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
    Print(L"Hola\r\n");
    return EFI_SUCCESS;
}
```

La lógica es idéntica — misma `EFI_SYSTEM_TABLE`, mismo mecanismo de `OutputString` — pero EDK2 agrega macros (`EFIAPI`, `IN`) y wrappers de biblioteca (`Print`) que hacen el código más portable entre arquitecturas y compiladores. El binario PE/COFF resultante es funcionalmente equivalente.

### Análisis en Ghidra: gnu-efi vs EDK2

La `Lesson_Ghidra` del repositorio muestra el análisis de un binario EDK2 en Ghidra. Comparado con nuestro binario gnu-efi, las diferencias observables son:

- **Símbolos:** EDK2 preserva más información de debug en el binario (archivos `.pdb`), lo que hace que Ghidra resuelva más nombres de funciones automáticamente.
- **Estructura interna:** EDK2 genera binarios más grandes con más secciones, ya que incluye el runtime completo de `MdePkg`. gnu-efi produce binarios más compactos (nuestro `aplicacion.efi` tiene solo 512 bytes de código).
- **Calling convention:** Ambos usan la misma convención de llamada x86-64 (Microsoft ABI), por lo que el patrón de uso de registros (`RCX`, `RDX`, `R8`, `R9`) es idéntico en el desensamblado de Ghidra.

---

## Flujo completo resumido

```
aplicacion.c
     │
     │  gcc (compilar, sin libc, PIC)
     ▼
aplicacion.o  (ELF relocatable)
     │
     │  ld (linker script UEFI + crt0)
     ▼
aplicacion.so (ELF shared object)
     │
     │  objcopy (reempaquetar secciones)
     ▼
aplicacion.efi (PE/COFF — ejecutable nativo UEFI)
```
