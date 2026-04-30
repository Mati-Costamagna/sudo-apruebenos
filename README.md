# Interfaz de Firmware Extensible Unificada (UEFI) 

**Asignatura:** Sistemas de Computación  

**Profesores:** 
  - Jorge, Javier Alejandro
  - Solinas, Miguel Angel

**Estudiantes:** 
  - Costamagna, Matias
  - Davila Tomassi, Carlos Valentino
  - Sabena, Maria Pilar

**Link del repositorio:** https://github.com/Mati-Costamagna/sudo-apruebenos/tree/TP3a

**Fecha:** Mayo 2026

---

## Introducción

Este trabajo es la continuación directa del trabajo anterior sobre **Modo Real y Modo Protegido**, donde se estudió cómo el procesador x86 arranca en Real Mode (16 bits, 1 MB de memoria, sin protección), se construyó un sector de arranque MBR en ensamblador y se ejecutó la transición a **Modo Protegido** configurando una GDT y activando el bit `PE=1` en `CR0`. Ese trabajo dejó al procesador en un entorno controlado de 32 bits, ejecutando código directamente sobre el hardware sin sistema operativo.

El presente trabajo parte de ese punto y da el siguiente paso: **¿qué hay entre el reset del procesador y el momento en que arranca el sistema operativo en una computadora moderna?** La respuesta es **UEFI** (Unified Extensible Firmware Interface), el estándar que reemplazó al BIOS Legacy y que opera en las fases previas al OS gestionando la inicialización del hardware, la memoria y los dispositivos de arranque.

### Lo que se hizo en el trabajo anterior

- Construcción de un MBR de 512 bytes en ensamblador (GAS/AT&T), con firma `0x55 0xAA`.
- Configuración de la **GDT** (Global Descriptor Table) para definir segmentos de código y datos.
- Transición de Real Mode a **Protected Mode de 32 bits** activando `PE=1` en `CR0`.
- Salida por VGA directa (sin OS, sin BIOS) desde Protected Mode.
- Ejecución en QEMU y en hardware físico (Lenovo T450).

### Lo que se busca en este trabajo

- Comprender la arquitectura de **Platform Initialization (PI)**: las fases SEC, PEI, DXE, BDS y RT que organizan el arranque moderno.
- Desarrollar una **aplicación nativa UEFI** en C usando `gnu-efi`, compilarla al formato **PE/COFF** mediante un pipeline de tres etapas (`gcc` → `ld` → `objcopy`), y ejecutarla en el entorno pre-OS.
- Analizar el binario resultante con herramientas de ingeniería inversa (**Ghidra**) para entender cómo un descompilador interpreta opcodes a nivel de firmware, en particular la instrucción `INT3` (`0xCC`) y su representación como `-52` en complemento a dos.
- Conectar los conceptos del trabajo anterior (modos del procesador, MBR, formato de ejecutables bare-metal) con su equivalente moderno en el ecosistema UEFI.

---

## Estructura del repositorio

| Directorio | Contenido |
|---|---|
| `parte1/` | TP1: Exploración del entorno UEFI y la Shell (comandos `map`, `dh`, `dmpstore`, `memmap`) |
| `parte2/` | TP2: Desarrollo, compilación y análisis de seguridad |
| `parte3/` | TP3: Ejecución en hardware físico (bare metal, USB booteable) |

---

## TP1: Exploración del entorno UEFI y la Shell

**Objetivo:** Explorar cómo UEFI abstrae el hardware y gestiona la configuración antes de la carga del sistema operativo.

### Arranque en el entorno virtual

Para explorar el entorno UEFI sin hardware específico, se usó **QEMU** como emulador y **OVMF** como firmware:

```bash
qemu-system-x86_64 -m 512 -bios /usr/share/ovmf/OVMF.fd -net none
```

A diferencia del BIOS Legacy que simplemente saltaba al MBR en `0x7C00`, UEFI arranca con una shell interactiva completa (**UEFI Interactive Shell v2.2**) con su propio sistema de archivos, consola y gestión de memoria.

### Exploración de Handles y Protocolos

UEFI no usa puertos de hardware fijos ni interrupciones como el BIOS. En cambio mantiene una base de datos de **Handles** (identificadores de entidades) que agrupan **Protocolos** (interfaces de software identificadas por GUIDs).

![Fases UEFI](parte1/assets/tp1.svg)

```
Shell> map
Shell> dh -b
```
![map y dh](parte1/assets/dhb.png)

El comando `map` mostró el único dispositivo de bloque (`BLK0`) con su ruta completa en el árbol PCI: `PciRoot(0x0)/Pci(0x1,0x1)/Ata(0x0)`. El comando `dh -b` listó todos los Handles del sistema: `DxeCore`, `RuntimeArch`, `CpuArch`, `SecurityArch`, `DebugSupport`, entre otros.

### Variables de NVRAM y secuencia de arranque

La fase BDS decide qué cargar basándose en variables no volátiles almacenadas en NVRAM:

```
Shell> dmpstore -b
Shell> set TestSeguridad "Hola UEFI"
Shell> set -v
```

![dmpstore](parte1/assets/dmpstore.png)

Se observaron las variables `BootOrder` (`00 00 01 00`) y `Boot0001` apuntando a la UEFI Shell interna. El Boot Manager lee `BootOrder`, itera cada entrada `Boot####` en orden y llama a `LoadImage()` con la Device Path correspondiente hasta encontrar un binario válido.

### Mapa de memoria y hardware

```
Shell> memmap -b
Shell> pci -b
Shell> drivers -b
```
![memmap](parte1/assets/memmap.png)

El mapa de memoria reveló las regiones clave: `BS_Code` (990 páginas, liberadas al cargar el OS), `RT_Code` (256 páginas, **permanecen mapeadas en el OS**) y `RT_Data` (481 páginas). Los comandos `pci` y `drivers` listaron los 5 dispositivos PCI emulados y los drivers cargados por DXE (`PciBusDxe`, `DiskIoDxe`, `PartitionDxe`, `SataController`, `GraphicsConsoleDxe`, entre otros).

### Hallazgo clave: regiones RuntimeServicesCode como vector de ataque

Las regiones `RT_Code` son el objetivo principal de los **bootkits** porque sobreviven a `ExitBootServices()`, ejecutan en Ring 0 con acceso total a la memoria, y son invisibles para cualquier software de seguridad que corra dentro del OS. Más aún, ciertas implementaciones aprovechan el **System Management Mode (SMM)** — un modo de ejecución en "Anillo -2", por debajo incluso del hipervisor — completamente transparente para el OS. Bootkits reales como *LoJax* (2018) y *CosmicStrand* (2022) usaron exactamente este vector para lograr persistencia a nivel de firmware.


> Ver desarrollo completo en [`parte1/README.md`](parte1/README.md)

---

## TP2: Desarrollo, compilación y análisis de seguridad

**Objetivo:** Crear una aplicación nativa UEFI en C, compilarla al formato PE/COFF y analizar cómo un descompilador interpreta sus opcodes a nivel de firmware.

### Contexto: dónde corre el código

![Fases PI](parte2/assets/pi_boot_phases.svg)

La aplicación desarrollada corre en la fase **BDS**, luego de que **DXE** estableció el entorno de 64 bits con GDT y servicios UEFI activos. El procesador ya transitó por Real Mode (SEC) → Protected Mode 32-bit (PEI) → Long Mode 64-bit (DXE), el mismo recorrido estudiado en el trabajo anterior pero gestionado ahora por el firmware UEFI en lugar de nuestro propio código ensamblador.

### Pipeline de compilación

El binario no se compila con un simple `gcc` — UEFI requiere el formato **PE/COFF** (el mismo de los `.exe` de Windows), lo que impone un proceso de tres etapas:

```
aplicacion.c
     │  gcc  (-ffreestanding, -fpic, sin libc)
     ▼
aplicacion.o  (ELF relocatable)
     │  ld   (linker script UEFI + crt0)
     ▼
aplicacion.so (ELF shared object)
     │  objcopy  (reempaquetar secciones)
     ▼
aplicacion.efi (PE/COFF — ejecutable nativo UEFI)
```

A diferencia del MBR del trabajo anterior (512 bytes crudos, cargado siempre en `0x7C00`), el PE/COFF incluye una tabla de reubicaciones `.reloc` que permite al firmware cargarlo en cualquier dirección física disponible (`ImageBase = 0x0`).

### Análisis con Ghidra

El binario compilado se importó en **Ghidra** para analizar cómo el descompilador interpreta el código a nivel de firmware.

![Ghidra efi_main](parte2/assets/ghidra_efi_main.png)

El panel izquierdo muestra el desensamblado x86-64 con las variables de stack y sus referencias cruzadas (XREF). El panel derecho muestra el pseudocódigo reconstruido, donde `SystemTable->ConOut->OutputString` aparece como una doble derreferencia de puntero: `(**(...)(unaff_RSI + 0x40) + 8))`.

#### Hallazgo clave: `0xCC` = `-52`

La aplicación embebe el opcode `0xCC` (instrucción `INT3`, breakpoint de software) en un array y lo compara en tiempo de ejecución. Al compilar con `-O0` para desactivar optimizaciones, el `CMP` y el `JNZ` son visibles en el desensamblado. Al aplicar **Convert → Signed Decimal** en Ghidra sobre el operando, `0xCC` se representa como `-52`:

![Ghidra CMP -52](parte2/assets/ghidra_cmp_signed_52.png)

`0xCC` (204 sin signo) = `-52` en complemento a dos de 8 bits, porque el bit más significativo está en 1. Este fenómeno es relevante en ciberseguridad: un analista que no reconozca la equivalencia `-52 ↔ 0xCC ↔ INT3` puede pasar por alto técnicas de anti-debugging o breakpoints intencionales en malware de firmware.

> Ver análisis completo en [`parte2/README.md`](parte2/README.md)

---

## TP3: Ejecución en hardware físico (bare metal, USB booteable)

---