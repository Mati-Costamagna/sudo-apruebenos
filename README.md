# sudo-apruebenos 🖥️ Sistemas de Computación

![Multi-Branch](https://img.shields.io/badge/Branches-6%20TPs-blue)
![Languages](https://img.shields.io/badge/Languages-C%20%7C%20Assembly%20%7C%20Python-orange)
![Status](https://img.shields.io/badge/Status-Completado-success)
![License](https://img.shields.io/badge/License-MIT-green)

## 📋 Descripción

Repositorio que contiene la **implementación completa de los trabajos prácticos** de la asignatura **Sistemas de Computación**, parte fundamental de la carrera de **Ingeniería en Computación** en la **FCEFyN - Universidad Nacional de Córdoba**.

Cada rama (`TP1`, `TP2`, `TP3`, `TP4`, `TP5`, etc.) contiene la implementación de un trabajo práctico distinto, permitiendo explorar diferentes aspectos de **arquitectura de computadoras**, **programación de bajo nivel** y **desarrollo de drivers de kernel**.

---

## 👥 Autores

| Integrante | GitHub | Email |
|---|---|---|
| **Matias Javier Costamagna** | [@Mati-Costamagna](https://github.com/Mati-Costamagna) | matias.costamagna@mi.unc.edu.ar |
| **Maria Pilar Sabena** | [@pilarsabena](https://github.com/pilarsabena) | maria.sabena@mi.unc.edu.ar |
| **Carlos Valentino Davila Tomassi** | [@asderu18](https://github.com/asderu18) | carlos.davila@mi.unc.edu.ar |

---

## 🎯 Objetivos de la Asignatura

- ✅ Entender la **arquitectura de computadoras** a nivel profundo
- ✅ Programación en **C de bajo nivel** y **Assembly**
- ✅ Comprensión de **convenciones de llamadas** y **stack frames**
- ✅ Desarrollo de **device drivers** para Linux kernel
- ✅ Análisis de **rendimiento** y **optimización**
- ✅ Interacción con **firmware** (BIOS/UEFI)
- ✅ Programación de **módulos del kernel**

---

## 📂 Estructura de Ramas (TPs)

### 🔵 **TP1** - Rendimiento de Computadoras
**Rama:** `TP1`

**Objetivo Principal:**  
Comprender y medir el rendimiento de un sistema computacional mediante diferentes métricas.

**Temas Cubiertos:**
- Definición de rendimiento (inversamente proporcional al tiempo)
- Medidas de desempeño: CPI, IPC, Throughput, Latencia
- Parámetros que afectan rendimiento: frecuencia CPU, CPI, número de instrucciones
- Speedup y Eficiencia
- MIPS y FLOPS
- Ley de Amdahl
- **Práctico:** Medición empírica variando frecuencia de CPU (ESP32)
- Programas benchmark (sintéticos, reducidos, kernel, reales)
- Profiling de código (gprof, perf, heaptrack)

**Tecnologías:** C, Python, ESP32/ARM, herramientas de profiling

---

### 🟢 **TP2** - Convenciones de Llamadas y Stack Frames
**Rama:** `TP2`

**Objetivo Principal:**  
Entender profundamente cómo los lenguajes de alto nivel (C/Python) interactúan con lenguajes de bajo nivel (Assembly) mediante convenciones de llamadas.

**Temas Cubiertos:**
- **Capas de arquitectura:** Hardware → Assembly → C → Python
- **Convención de llamadas System V AMD64 ABI**
  - Registros: `%rdi`, `%rsi`, `%rdx`, `%rcx`, `%r8`, `%r9` para argumentos
  - Registro `%rax` para valor de retorno
  - Registros callee-saved (`%rbp`, `%rsp`)
- **Stack Frame:**
  - Estructura de la pila (LIFO)
  - Prólogo y epílogo de funciones
  - Variables locales y dirección de retorno
- **Integración C ↔ Assembly:**
  - Consumo de API REST (libcurl en C, requests en Python)
  - Índice GINI del Banco Mundial como caso práctico
  - Llamadas cruzadas: Python → C → Assembly
- **Depuración con GDB:**
  - Inspección de registros y memoria
  - Análisis de stack frames
  - Breakpoints y stepping

**Práctico:**
- Implementar función `suma()` en Assembly, llamarla desde C
- Compilación manual: `as`, `gcc`, `ld`
- Depuración paso a paso mostrando estado de registros y stack
- Consumir API REST y procesar datos con conversiones float→int

**Tecnologías:** C, Assembly (AT&T/Intel syntax), Python, libcurl, GDB

---

### 🟡 **TP3** - Modo Real, Modo Protegido y Bootloader
**Rama:** `TP3`

**Objetivo Principal:**  
Entender los modos de operación x86 y cómo funciona el proceso de arranque desde BIOS hasta modo protegido.

**Temas Cubiertos:**
- **Modos de funcionamiento x86:**
  - Real Mode (16-bit)
  - Protected Mode (32-bit)
  - IA-32e Mode (64-bit)
  - System Management Mode (SMM)
- **Segmentación:**
  - Segmentación en modo real vs. protegido
  - Registros de segmento: `CS`, `DS`, `SS`, `FS`, `GS`
  - Cálculo de direcciones: `16 * Segment + Offset`
- **Bootloader y Sector de Arranque:**
  - Creación de imagen MBR booteable
  - Requisitos: `\125\252` (0x55AA) al final del sector
  - Instrucción `hlt` (0xf4)
- **BIOS/UEFI:**
  - Interrupciones BIOS (int)
  - Paso de parámetros por registros
  - Funciones disponibles en modo real
- **Transición a Modo Protegido:**
  - Global Descriptor Table (GDT)
  - Registro CR0 (PE bit - Protection Enable)
  - Salto lejano con `ljmp`
- **Memoria de Video:**
  - Mapeo de memoria: 0xB8000
  - Impresión en pantalla directamente
- **Emulación y Debugging:**
  - QEMU con bootloader
  - GDB + QEMU para depuración
  - Análisis con `objdump`, `hexdump`

**Práctico:**
1. Crear imagen MBR con `printf '\364%509s\125\252'`
2. Bootloader en Assembly que imprime mensaje
3. Transición a modo protegido sin macros
4. Crear descriptores de memoria diferenciados
5. Configurar segmento de datos como read-only y verificar con GDB
6. Grabar en USB y probar en PC real

**Tecnologías:** Assembly (NASM/GAS), QEMU, GDB, Linker scripts

---

### 🟣 **TP3a** - UEFI (Interfaz de Firmware Extensible Unificada)
**Rama:** `TP3a`

**Objetivo Principal:**  
Entender UEFI como la evolución moderna del BIOS, con arquitectura modular pre-OS.

**Temas Cubiertos:**
- **UEFI vs. Legacy BIOS:**
  - Limitaciones del BIOS: 16-bit, 1MB memoria, hardware heredado
  - Ventajas de UEFI: 32/64-bit, modular, extensible
- **Fases de Platform Initialization (PI):**
  1. **SEC** (Security): Pre-memoria, Cache-as-RAM
  2. **PEI** (Pre-EFI Init): Inicialización de hardware mínimo
  3. **DXE** (Driver Execution Environment): Carga de drivers
  4. **BDS** (Boot Device Selection): Selección de dispositivo de arranque
  5. **RT** (Runtime): Servicios después de cargar OS
- **Estructuras Centrales:**
  - UEFI System Table (entry point)
  - Boot Services y Runtime Services
  - Protocolos y Handles (abstracción de hardware)
  - GUIDs (identificadores únicos)
- **UEFI Driver Model:**
  - Separación carga/activación
  - Lazy activation (fast boot)
  - Driver Binding Protocol
- **Seguridad:**
  - Secure Boot (verificación de firmas X509)
  - S3 Resume (Suspend to RAM) y boot scripts
  - Ataques potenciales
- **Formato PE/COFF:**
  - Imágenes UEFI cargadas dinámicamente
  - Relocation fix-ups
  - LoadImage() y ejecución

**Práctico:**
1. **Entorno:** QEMU + OVMF firmware
2. **Exploración UEFI Shell:**
   - Comando `map`: ver handles y protocolos
   - `dh -b`: bases de datos de dispositivos
   - `dmpstore`: variables NVRAM
3. **Desarrollo:**
   - Aplicación UEFI en C usando `gnu-efi`
   - Compilación a formato PE/COFF
   - Conversión: `.so` → `.efi` con `objcopy`
4. **Análisis:**
   - Descompilación con Ghidra
   - Análisis de pseudocódigo
5. **Depuración:**
   - GDB + QEMU (puerto 1234)
   - Ghidra Debugger integrado
6. **Hardware:**
   - Crear USB UEFI booteable
   - Ejecutar en Lenovo T450 (desactivar Secure Boot)
   - Ejecución bare metal fuera del SO

**Tecnologías:** C, gnu-efi, PE/COFF, Ghidra, GDB, QEMU con OVMF

---

### 🔴 **TP4** - Módulos del Kernel Linux
**Rama:** `TP4`

**Objetivo Principal:**  
Entender cómo escribir código que forma parte del kernel Linux y gestiona hardware.

**Temas Cubiertos:**
- **Conceptos Fundamentales:**
  - ¿Qué son los módulos del kernel?
  - Diferencia entre programas y módulos
  - Espacio de usuario vs. espacio de kernel
- **Estructura de Módulos:**
  - Función `module_init()` (constructor)
  - Función `module_exit()` (destructor)
  - Compilación contra headers del kernel
- **Carga/Descarga:**
  - `insmod`: insertar módulo
  - `lsmod`: listar módulos
  - `rmmod`: remover módulo
  - Verificación en `dmesg`
- **Sistema de Archivos `/proc`:**
  - Interfaz virtual del kernel
  - `/proc/cpuinfo`: información del procesador
  - `/proc/modules`: módulos cargados
  - Crear entradas `/proc` personalizadas
- **Espacio de Nombres (Namespace):**
  - Conflictos de símbolos globales
  - `/proc/kallsyms`: símbolos accesibles del kernel
  - Macros `EXPORT_SYMBOL`
- **Dispositivos:**
  - `/dev`: archivos de dispositivos
  - Tipos: character, block, network
- **API de Linux:**
  - `open()`, `close()`, `read()`, `write()`, `lseek()`
  - Descriptores de archivo
  - Gestión de memoria (stack, heap)

**Práctico:**
1. Instalar tools: `gcc`, `build-essential`, `linux-source`, `linux-headers`
2. Escribir primer módulo (`mimodulo.c`)
3. Compilar, insertar y verificar en `dmesg`
4. Examinar `/proc/modules`
5. Analizar `/proc/cpuinfo` y otros archivos del sistema
6. Explorar `/lib/modules/$(uname -r)/kernel`

**Tecnologías:** C, Linux kernel API, Make

---

### 🟠 **TP5** - Device Drivers (Character Device Driver)
**Rama:** `TP5`

**Objetivo Principal:**  
Desarrollar device drivers completos que controlen hardware mediante el kernel Linux.

**Temas Cubiertos:**
- **Clasificación de Drivers:**
  - Device Driver vs. Driver (general)
  - Character Device Driver (CDD): bytes, puertos seriales, audio, cámaras
  - Block Device Driver: almacenamiento, E/S en bloques
  - Network Driver: paquetes, protocolos
- **Arquitectura de Hardware:**
  - Device Controller
  - Bus Controller
  - Abstracción de hardware
- **Números Major y Minor:**
  - Vínculo entre CDF (Character Device File) y CDD
  - Tipo `dev_t` y macros: `MAJOR()`, `MINOR()`, `MKDEV()`
  - Consulta: `cat /proc/devices`
- **Registro de Drivers:**
  - `register_chrdev_region()`: rango estático
  - `alloc_chrdev_region()`: rango dinámico
- **Creación de Archivos de Dispositivo:**
  - `mknod`: creación manual
  - `/sys/class`: información del dispositivo
  - `udev`: demonio que crea automáticamente archivos
  - `device_create()` y `device_destroy()`
- **Operaciones del Driver:**
  - Estructura `file_operations`
  - Funciones: `open()`, `close()`, `read()`, `write()`
  - Valores de retorno: bytes leídos/escritos o error
- **Sistema `/sys`:**
  - Pseudo-filesystem que expone estructuras del kernel
  - Información en `/sys/class/net/`, `/sys/devices/`
- **Módulo Clipboard:**
  - Ejemplo de integración con `/proc`
  - Estructura `proc_ops` (recomendada en kernels nuevos)
- **Seguridad de Módulos:**
  - Secure Boot y firma de módulos
  - Evitar cargar módulos no firmados

**Práctico:**
1. **drv1.c:** Primer módulo básico
2. **drv2.c:** Registro de driver (major/minor)
   - Compilar, insertar, verificar en `/proc/devices`
   - Crear archivos con `mknod`
3. **drv3.c:** Decodificar operaciones (`open`, `close`, `read`, `write`)
   - Usar `cat` y `echo` para probar
   - Analizar retorno de funciones en `dmesg`
4. **drv4.c:** Mejorar `read()` y `write()`
   - Copiar datos a espacio de usuario
   - Verificar con `strace`
5. **clipboard.c:** Módulo con entrada `/proc`
   - Crear `/proc/clipboard`
   - Escribir y leer datos
6. **Integración con Hardware:**
   - GPIO mapeados en memoria
   - Ejemplo: Raspberry Pi

**Tecnologías:** C, Linux kernel API, Make, strace, udev, Raspberry Pi (opcional)

---

### 🏗️ **stack_frame** - Análisis Profundo de Stack Frames
**Rama:** `stack_frame`

**Objetivo Especial:**  
Análisis detallado de cómo funciona el stack durante la ejecución de funciones.

**Contenido:**
- Análisis paso a paso de stack frames
- Visualización con GDB
- Comprensión de `%rbp` (base pointer) y `%rsp` (stack pointer)
- Estructura de memoria durante llamadas a funciones
- Acceso a parámetros y variables locales desde el stack
- Convención de llamadas en x86-64

**Tecnologías:** Assembly, GDB, análisis manual de memoria

---

## 🚀 Cómo Usar este Repositorio

### 1️⃣ Clonar el Repositorio

```bash
git clone https://github.com/Mati-Costamagna/sudo-apruebenos.git
cd sudo-apruebenos
```

### 2️⃣ Listar Todas las Ramas Disponibles

```bash
git branch -a
```

**Output esperado:**
```
  TP1
  TP2
  TP3
  TP3a
  TP4
  TP5
  main
  prueba
  stack_frame
```

### 3️⃣ Cambiar a una Rama de TP Específica

```bash
# Ejemplo: acceder al TP1 (Rendimiento)
git checkout TP1

# O clonar directamente una rama
git clone --branch TP1 https://github.com/Mati-Costamagna/sudo-apruebenos.git tp1-folder
```

### 4️⃣ Compilar y Ejecutar

**Caso general (C/Assembly):**

```bash
# Compilar
gcc -o program program.c
./program

# O con Assembly
as -o program.o program.s
ld -o program program.o
./program

# O con Makefile
make
make run
```

**Casos especiales:**

**TP2 (compilación manual Assembly + C):**
```bash
as --64 -g -o suma.o suma.s
gcc -g -O0 -c -o main.o main.c
gcc -o programa main.o suma.o
gdb ./programa
```

**TP3 (Bootloader con QEMU):**
```bash
printf '\364%509s\125\252' > main.img
qemu-system-x86_64 --drive file=main.img,format=raw,index=0,media=disk
```

**TP4 (Módulos del kernel):**
```bash
make
sudo insmod mimodulo.ko
lsmod | grep mimodulo
dmesg | tail
sudo rmmod mimodulo
```

**TP5 (Device Drivers):**
```bash
make
sudo insmod drv5.ko
sudo mknod /dev/drv5_0 c $(grep drv5 /proc/devices | cut -d' ' -f1) 0
echo "Hola" > /dev/drv5_0
cat /dev/drv5_0
```

---

## 📚 Requisitos por TP

| TP | Requisitos |
|-----|-----------|
| **TP1** | GCC, Python, herramientas profiling (gprof, perf), ESP32 (opcional) |
| **TP2** | GCC, NASM/GAS, GDB, libcurl, Python |
| **TP3** | NASM/GAS, QEMU, GDB, Linker, objdump |
| **TP3a** | QEMU, OVMF, gnu-efi, Ghidra, GCC con soporte x86_64 |
| **TP4** | GCC, Linux kernel headers, Make |
| **TP5** | GCC, Linux kernel headers, Make, udev |
| **stack_frame** | GDB, Assembly knowledge, GCC |

---

## 🔧 Herramientas Principales

### Compilación y Ensamblado
- **GCC** - Compilador C
- **NASM / GAS** - Ensambladores
- **Make** - Automatización
- **Linker (ld)** - Linkeo manual

### Depuración
- **GDB** - Debugger GNU
- **GDB Dashboard** - Interfaz mejorada de GDB
- **Ghidra** - Ingeniería inversa y descompilación

### Emulación
- **QEMU** - Emulador de máquinas
- **OVMF** - Firmware UEFI para QEMU

### Análisis
- **objdump** - Desensambla ejecutables
- **strace** - Traza llamadas al sistema
- **perf** - Análisis de rendimiento
- **heaptrack** - Análisis de memoria

### Hardware
- **Raspberry Pi** (TP5 opcional)
- **ESP32** (TP1 opcional)

---

## 📖 Recursos Educativos

### Libros
- "Computer Organization and Design" - Patterson & Hennessy
- "The C Programming Language" - Kernighan & Ritchie
- "Lenguaje Ensamblador para PC" - Paul A. Carter
- "The Linux Kernel Module Programming Guide" - Salzman et al.

### Documentación Online
- [GCC Manual](https://gcc.gnu.org/onlinedocs/)
- [GDB Documentation](https://sourceware.org/gdb/documentation/)
- [x86-64 ISA](https://www.felixcloutier.com/x86/)
- [Linux Kernel Documentation](https://www.kernel.org/doc/html/latest/)
- [UEFI Specification](https://uefi.org/specifications)

### Tutoriales
- MIT OpenCourseWare - Computer Architecture
- UC Berkeley - CS61C
- OSdev.org - OS Development
- Linux Kernel Labs

---

## 🎓 Contexto Académico

- **Asignatura:** Sistemas de Computación
- **Carrera:** Ingeniería en Computación
- **Universidad:** FCEFyN - Universidad Nacional de Córdoba
- **Período:** 2025-2026

---

## 💡 Flujo Recomendado de Aprendizaje

```
TP1 (Rendimiento)
    ↓
TP2 (Stack Frames + Convenciones)
    ↓
TP3 (Modo Real/Protegido)
    ↓
TP3a (UEFI)
    ↓
TP4 (Módulos Kernel)
    ↓
TP5 (Device Drivers)
    ↓
stack_frame (Análisis Profundo)
```

---

## 📝 Estructura General de Cada TP

Cada rama típicamente contiene:

```
TP*/
├── src/                    # Código fuente
│   ├── *.c
│   ├── *.s (Assembly)
│   └── Makefile
├── include/               # Headers
├── docs/                  # Documentación, diagramas, referencias
├── results/              # Resultados de ejecución, benchmarks
└── README.md            # Documentación específica del TP
```

---

## 🔗 Enlaces Útiles

- [Repositorio](https://github.com/Mati-Costamagna/sudo-apruebenos)
- [Ramas disponibles](https://github.com/Mati-Costamagna/sudo-apruebenos/branches)
- [Ejemplos relacionados de Paul Carter](http://pacman128.github.io/pcasm/)
- [TianoCore EDK2 (UEFI)](https://github.com/tianocore/edk2)
- [Linux Kernel Source](https://github.com/torvalds/linux)

---

## 📄 Licencia

Este proyecto se distribuye bajo la licencia MIT.

---

## 💬 Notas Finales

- Cada TP es **independiente** pero construye sobre conceptos previos
- Se recomienda seguir el **flujo de aprendizaje** recomendado
- **Documenta todo** lo que hagas: es crucial para aprender
- Usa **GDB extensivamente** - es la herramienta más poderosa
- **Experimenta**: modifica código, observa qué cambia
- Los **screenshots y logs** son evidencia de tu entendimiento

---

**💡 Tip:** Usa `git log` en cada rama para ver el historial de desarrollo de cada TP.

---

**Última actualización:** Junio 2026  
**Estado:** ✅ Completado
