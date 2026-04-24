# Modo Protegido

**Asignatura:** Sistemas de Computación  

**Profesores:** 
  - Jorge, Javier Alejandro
  - Solinas, Miguel Angel

**Estudiantes:** 
  - Costamagna, Matias
  - Davila Tomassi, Carlos Valentino
  - Sabena, Maria Pilar

**Link del repositorio:** https://github.com/Mati-Costamagna/sudo-apruebenos/tree/TP3

**Fecha:** Abril 2026

---

## 1. Introduccion

Los procesadores x86 mantienen compatibilidad con sus antecesores mediante un proceso de evolucion durante el arranque. Al energizarse, todo CPU x86 comienza en modo real para garantizar compatibilidad hacia atras con el 8086 original, operando en 16 bits con acceso directo a solo 1 MB de memoria fisica y sin ningun mecanismo de proteccion entre procesos.

El problema fundamental del modo real es que cualquier programa puede leer o escribir cualquier direccion de memoria, incluyendo la del propio sistema operativo. Esto hace imposible construir un sistema multitarea estable: un solo proceso con un bug puede corromper todo el sistema. El modo real era aceptable cuando una computadora ejecutaba un solo programa a la vez, pero se volvio insostenible con el avance del software.

El modo protegido resuelve exactamente ese problema. Introducido con el 80286, fue el primer intento de agregar hardware especifico para aislar procesos entre si y del sistema operativo. Sin embargo, la implementacion del 286 era incompleta: una vez en modo protegido, no habia forma de volver a modo real sin resetear el procesador, lo que lo hizo poco practico. Fue el 80386 quien consolido el modelo que usamos hasta hoy, agregando paginacion, modos de 32 bits completos y la posibilidad de retornar a modo real mediante software.

Sus caracteristicas principales son:

- Proteccion de memoria: los programas no pueden acceder a zonas de memoria que no les corresponden. El hardware verifica cada acceso.
- Memoria virtual: soporte por hardware para memoria que excede la RAM fisica, mediante paginacion.
- Multitarea: conmutacion de tareas gestionada por hardware con contextos separados.
- Niveles de privilegio (rings): separacion entre codigo del kernel y codigo de usuario, con el hardware actuando como arbitro.

El objetivo de este TP es comprender y ejecutar la transicion desde modo real a modo protegido en un procesador x86, utilizando QEMU como entorno de virtualizacion.

---

## 2. Entorno de Trabajo

### 2.1 Herramientas utilizadas

```bash
sudo apt install qemu-system-x86 nasm binutils gdb
```

### 2.2 Sintaxis AT&T vs Intel (GAS vs NASM)

Existen dos ensambladores predominantes para x86: **NASM** (que usa sintaxis Intel) y **GAS** (que usa sintaxis AT&T, tambien llamada sintaxis Unix). Ambos generan el mismo codigo maquina, pero con convenciones opuestas. Conocer las diferencias es esencial porque la mayoria de ejemplos de bootloaders y kernels mezclan ambas tradiciones.
 
| Caracteristica | Intel (NASM) | AT&T (GAS) |
|---|---|---|
| Orden operandos | `mov dst, src` | `mov src, dst` |
| Inmediatos | `push 4` | `pushl $4` |
| Registros | `eax` | `%eax` |
| Tamaño de operando | prefijo en memoria (`byte ptr`) | sufijo en opcode (`movb`, `movw`, `movl`) |
| Comentarios | `;` | `#` o `/* */` |
 
La diferencia mas critica en la practica es el orden de operandos: en Intel el destino va primero, en AT&T va ultimo. Leer codigo AT&T con la intuicion de Intel (o viceversa) lleva a interpretar exactamente al reves que registro se esta modificando.

**Ejemplos comparativos:**

```asm
; Intel / NASM
mov eax, 4          ; EAX ← 4
mov al, byte ptr foo
push 4
 
# AT&T / GAS
movl $4, %eax       # EAX ← 4
movb foo, %al
pushl $4
```

### 2.3 Imagen booteable MBR

En arquitectura x86 lo mas simple es crear un sector de arranque MBR (Master Boot Record). El MBR ocupa exactamente 512 bytes y termina obligatoriamente con la firma `0x55 0xAA`.
 
```bash
printf '\364%509s\125\252' > src/main.img
```
 
Desglose del comando:
 
- `\364` (octal) = `0xF4` (hex) = instruccion `hlt` (detener CPU)
- `%509s` = 509 espacios para completar hasta el byte 510
- `\125\252` (octal) = `0x55 0xAA` = firma de sector booteable
Esta firma es lo **unico** que la BIOS verifica antes de ejecutar el contenido del sector. No hay ninguna validacion del codigo en si: si los ultimos dos bytes son `0x55 0xAA`, la BIOS transfiere el control incondicionalmente a `0x7C00`. Esto explica por que un sector completamente vacio excepto por esos dos bytes es tecnicamente "booteable", y tambien por que el malware de bootkit puede sobrevivir a una reinstalacion del sistema operativo: infecta el MBR antes de que cualquier software de seguridad del OS este activo.
 
**Estructura del MBR clasico:**
 
| Direccion (hex) | Tamaño (bytes) | Descripcion |
|---|---|---|
| 0x000 | 446 | Bootstrap code area |
| 0x1BE | 16 | Partition entry #1 |
| 0x1CE | 16 | Partition entry #2 |
| 0x1DE | 16 | Partition entry #3 |
| 0x1EE | 16 | Partition entry #4 |
| 0x1FE | 2 | Boot signature (0x55 0xAA) |
 
Para verificar el contenido de la imagen:
 
```bash
hd src/main.img
```
 
Para obtener la codificacion hexadecimal de una instruccion:
 
```bash
echo hlt > src/a.S
as -o src/a.o src/a.S
objdump -S src/a.o
```

### 2.4 Ejecucion de la imagen con QEMU

```bash
sudo apt install qemu-system-x86
qemu-system-x86_64 --drive file=src/main.img,format=raw,index=0,media=disk
```

![Salida de QEMU](assets/mbr_hlt.png)

### 2.5 Grabado en hardware real y problema con UEFI

Se intento grabar la imagen en un pendrive y bootear desde hardware real siguiendo el procedimiento estandar:

```bash
sudo dd if=src/main.img of=/dev/sda
```

![Grabado pendrive](assets/image.png)

Para verificar que la imagen fue grabada correctamente se ejecutaron los siguientes comandos:

```bash
# Verificar que el pendrive fue reconocido por el sistema
lsblk

# Comparar byte a byte la imagen con el dispositivo
sudo cmp -n 512 src/main.img /dev/sda

# Verificar la firma 0x55 0xAA en los bytes 510-511 del dispositivo
sudo dd if=/dev/sda bs=1 skip=510 count=2 | xxd
```
![Comparacion imagenes](assets/image_cmp.png)

La verificacion confirmo que la imagen fue grabada correctamente: los primeros 512 bytes del pendrive eran identicos a main.img y la firma 0x55 0xAA estaba presente en los offsets 0x1FE–0x1FF.
Sin embargo, al intentar bootear desde el pendrive, la UEFI de la maquina no detecto el dispositivo como booteable.

Luego de invesigar (un laaargo rato), encontramos que la razon es conceptual y no un error del procedimiento: una imagen MBR con bootloader de 16 bits no es reconocida como dispositivo booteable por UEFI.
El estandar UEFI espera una estructura completamente distinta, pero mas adelante se abordara este tema.

---

## 3. Linker

### ¿Que es un linker y que hace?

Un **linker** (enlazador) es una herramienta que combina uno o mas archivos objeto (`.o`) generados por el ensamblador o compilador, resuelve las referencias simbolicas entre ellos y produce un ejecutable final o imagen binaria.
 
Sus tareas principales son:
 
- **Resolucion de simbolos**: conecta cada referencia a una etiqueta con su definicion real.
- **Relocalizacion**: ajusta las direcciones de memoria de instrucciones y datos segun donde se cargara el programa. Esta es la tarea mas critica para codigo bare-metal.
- **Combinacion de secciones**: une secciones `.text`, `.data`, `.bss` de distintos objetos.
- **Formato de salida**: puede producir ELF, PE, binario plano, etc.
En desarrollo de software convencional el linker trabaja en conjunto con el sistema operativo, que carga los ejecutables en memoria y ajusta las relocalizaciones en tiempo de carga. En programacion bare-metal no hay sistema operativo que ayude: el linker debe producir un binario que funcione correctamente en la direccion exacta donde el hardware lo colocara.

### ¿Que es la direccion `0x7C00` en el script del linker?
 
```ld
SECTIONS {
    . = 0x7c00;
    .text : {
        __start = .;
        *(.text)
        . = 0x1FE;
        SHORT(0xAA55)
    }
}
```

`0x7C00` es la **direccion fisica donde el BIOS carga el sector de arranque (MBR) en RAM**. El valor no es arbitrario: en el IBM PC original con 32 KB de RAM, el equipo de IBM decidio ubicar el bootloader al final de la memoria disponible para maximizar el espacio libre contiguo hacia arriba para el sistema operativo. `0x7C00` es `32KB - 1KB - 512 bytes`: se reservo 1 KB para el stack del bootloader entre `0x7C00` y `0x7E00`, y el codigo ocupa los 512 bytes desde `0x7C00`.
 
Es **necesario** indicarsela al linker porque el codigo ensamblado contiene referencias absolutas (etiquetas, saltos, datos). Si el linker no sabe que el codigo se ejecutara desde `0x7C00`, calculara mal todas las direcciones absolutas. Por ejemplo, si la etiqueta `msg` esta a 20 bytes del inicio del codigo, su direccion real en ejecucion sera `0x7C00 + 20 = 0x7C14`, no `0x14`. Sin este dato, el bootloader intentaria leer el string de la direccion `0x14`, que contiene basura, y el resultado seria silenciosamente incorrecto.

### Comparacion `objdump` vs `hd`
 
Compilar y linkear el hello world (utilizado en la parte 5):
 
```bash
as -g -o src/main.o src/main.S
ld --oformat binary -o src/main.img -T src/link.ld src/main.o
```
 
Ver el desensamblado con direcciones ajustadas a `0x7C00`:
```bash
objdump -D -b binary -m i8086 -M addr16,data16 src/main.img
```

![Desensamblado](assets/objdump.png)
 
Ver el volcado hexadecimal crudo del archivo:
```bash
hd src/main.img
```

![Imagen](assets/hd_image.png)
 
La comparacion entre ambas herramientas es reveladora. `objdump` muestra las instrucciones con sus **direcciones logicas** (empezando en `0x7C00`), tal como las ve el procesador en ejecucion. `hd` muestra los **offsets dentro del archivo** (empezando en `0x0000`). Ambas vistas describen los mismos bytes, pero desde perspectivas distintas: la del procesador en ejecucion vs. la del archivo en disco. La firma `55 AA` debe aparecer en `hd` en el offset `0x01FE`, y en `objdump` en la direccion `0x7DFE`.

### `--oformat binary`

La opcion `--oformat binary` le indica al linker que genere un **archivo binario plano** (raw binary), sin ningun encabezado de formato ejecutable.
 
Esto es necesario porque el BIOS no entiende formatos como ELF: simplemeónte copia los 512 bytes del sector al RAM y salta a `0x7C00`. Los primeros bytes de un ELF son `0x7F 0x45 0x4C 0x46` ('ELF' en ASCII). Si el BIOS intentara ejecutarlos como codigo x86, `0x7F` es la instruccion `JNS` (jump if not sign), que saltaria a una direccion basura. El resultado seria un cuelgue inmediato o comportamiento completamente impredecible. Este es un buen ejemplo de por que en programacion bare-metal el **formato del binario importa tanto como su contenido**.

---

## 4. UEFI y Coreboot

### UEFI

**UEFI** (Unified Extensible Firmware Interface) es el sucesor moderno del BIOS. La diferencia fundamental no es solo tecnica sino arquitectural: mientras el BIOS es esencialmente un conjunto de rutinas en ensamblador de 16 bits que expone su funcionalidad mediante interrupciones de hardware, UEFI es un entorno completo que opera en modo protegido de 32 bits o modo largo de 64 bits desde el inicio, con una interfaz orientada a protocolos similar a una API de sistema operativo.
 
Caracteristicas principales:
 
- Soporte para discos mayores a 2 TB usando GPT en lugar de MBR.
- Interfaz de programacion orientada a protocolos, con llamadas a funciones en lugar de interrupciones.
- Drivers escritos en C compilados a bytecode PE/COFF.
- **Secure Boot**: verificacion criptografica de la cadena de arranque mediante firmas digitales.
- Shell UEFI interactivo para diagnostico y scripting.

**¿Como se usa programaticamente?**
 
Una aplicacion UEFI se escribe en C usando el EFI SDK (EDK2 o GNU-EFI). El punto de entrada recibe un puntero a la tabla de servicios del sistema, que es el equivalente a la tabla de interrupciones del BIOS pero en forma de tabla de punteros a funciones.

**Ejemplo de funcion invocable — `GetMemoryMap`:**
 
```c
EFI_MEMORY_DESCRIPTOR *MemMap = NULL;
UINTN MemMapSize = 0, MapKey, DescSize;
UINT32 DescVer;
 
uefi_call_wrapper(
    SystemTable->BootServices->GetMemoryMap, 5,
    &MemMapSize, MemMap, &MapKey, &DescSize, &DescVer
);
```

Esta funcion devuelve un mapa completo de la memoria fisica del sistema. Es equivalente a lo que en BIOS se obtenia con la interrupcion `INT 0x15, AX=0xE820`, pero en lugar de una interfaz de registros de 16 bits, es una llamada a funcion con parametros tipados. Todo kernel moderno necesita este mapa para saber que regiones de memoria puede usar libremente y cuales estan reservadas por el firmware.

### Casos de Bugs

- **[BootHole](https://www.genbeta.com/seguridad/boothole-error-critico-grub2-que-afecta-a-millones-sistemas-linux-windows-cuentan-arranque-dual) (2020)**: vulnerabilidad en GRUB2 que permitía eludir Secure Boot mediante un archivo de configuración malicioso. Afectó prácticamente a todos los sistemas Linux con Secure Boot habilitado, demostrando que la seguridad de la cadena de arranque es tan fuerte como su eslabón más débil.
- **[LogoFAIL](https://unaaldia.hispasec.com/2023/12/vulnerabilidades-criticas-en-uefi-logofail-expone-a-dispositivos-x86-y-arm.html) (2023)**: conjunto de vulnerabilidades en los parsers de imágenes del firmware UEFI (BMP, PNG, JPEG). Un atacante podía colocar una imagen maliciosa en la partición EFI para ejecutar código arbitrario antes de que el OS cargue, incluso antes de que Secure Boot tenga efecto. Irónicamente, las imágenes de logo que se muestran durante el arranque se convierten en un vector de ataque.
- **[BlackLotus](https://www.binarly.io/blog/the-untold-story-of-the-blacklotus-uefi-bootkit) (2023)**: primer bootkit UEFI in-the-wild para Windows capaz de persistir incluso tras reinstalar el sistema operativo o reemplazar el disco duro. BlackLotus lo demostró en la práctica: el malware sobrevivía a formateos completos porque no vivía en el disco, sino en la memoria flash de la placa madre.


### ¿Que es CSME e Intel MEBx?

**CSME (Converged Security and Management Engine)** es un subsistema dedicado embebido en los chipsets Intel modernos. Corre su propio microkernel (Minix 3) en un procesador separado con acceso directo a memoria, red y almacenamiento, operando de forma completamente independiente al CPU principal, incluso con el sistema apagado si hay corriente en el standby. Se usa para gestion remota (AMT), verificacion de integridad del firmware y arranque seguro de la plataforma.
 
La implicacion de seguridad es significativa: el CSME tiene mas privilegios que el sistema operativo, mas que el hipervisor, y su codigo es propietario y no auditable publicamente. En 2017, Intel-SA-00086 revelo vulnerabilidades que permitian ejecucion de codigo arbitrario en el ME con nivel de privilegio por debajo del OS y sin posibilidad de deteccion desde el sistema operativo.
 
**Intel MEBx (Management Engine BIOS Extension)** es la interfaz de configuracion del ME accesible durante el POST (presionando Ctrl+P). Permite al administrador configurar AMT: activar/desactivar gestion remota, establecer contraseñas, configurar acceso a red out-of-band. En entornos corporativos su correcta configuracion es critica, ya que AMT mal configurado expone acceso remoto completo al hardware.

### Coreboot 

**Coreboot** (antes LinuxBIOS) es un proyecto de firmware libre que reemplaza al BIOS/UEFI propietario. Su filosofia es hacer el minimo de inicializacion de hardware posible y luego transferir el control a un *payload* (GRUB2, SeaBIOS, TianoCore/UEFI, Linux directamente, etc.). El contraste con BIOS y UEFI es filosofico ademas de tecnico: coreboot asume que el firmware no deberia ser un sistema operativo en miniatura, sino un inicializador minimalista y auditables.
 
**Productos que lo incorporan:**
 
- **Chromebooks** de Google (toda la linea, siendo quizas el usuario mas masivo a escala global).
- **Purism Librem** (laptops orientadas a privacidad y seguridad).
- **System76** (laptops y desktops para Linux).
- **ASUS Chromebox / Chromebit**.
- Ciertos modelos de **Lenovo ThinkPad** con coreboot + Heads para mayor seguridad verificable.

**Ventajas de Coreboot:**
 
- **Codigo abierto y auditable**: cualquiera puede revisar que hace el firmware antes de que cargue el OS, algo imposible con BIOS/UEFI propietario.
- **Arranque extremadamente rapido**: en algunos sistemas logra tiempos de POST menores a 1 segundo, ya que elimina toda la inicializacion redundante que el BIOS tradicional hace por compatibilidad historica.
- **Superficie de ataque reducida**: al eliminar codigo propietario y reducir el tamaño del firmware, se reduce la cantidad de vectores de ataque posibles antes de que el OS tome el control.
- **Longevidad**: permite seguir usando hardware antiguo con firmware actualizado y seguro, sin depender del soporte del fabricante original.

---

## 5. Hello World en Modo Real

### El codigo `main.S`

Para personalizar un poco la actividad e intentar distinguirnos del resto, le hicimos una pequeña modificacion al codigo de la presentacion:

```asm
.code16                     # Codigo de 16 bits (modo real)
    mov $msg, %si           # SI apunta al string a imprimir
    mov $0x0e, %ah          # Función 0x0e de la INT 10h: Teletype Output
loop:
    lodsb
    or %al, %al             # Carga byte en AL desde [SI], incrementa SI
    jz halt
    int $0x10               # Setea flags; si AL == 0 → fin del string
    jmp loop                # Llamada a la BIOS: imprime caracter en AL
halt:
    hlt
msg:
    .asciz "hello SdeC"     # String terminado en '\0'
```

Este programa ilustra el modelo de programacion del modo real: para imprimir un caracter se usa la interrupcion `INT 0x10`, que es una de las rutinas del BIOS residente en ROM. El BIOS es esencialmente una biblioteca de funciones de bajo nivel accesible solo a traves de interrupciones de software, con argumentos pasados por registros. Esta interfaz existe desde el IBM PC original y es la razon por la que el BIOS solo puede usarse en modo real: las rutinas fueron escritas para operar con el modelo de segmentacion de 16 bits.
 
Una vez que el procesador pase a modo protegido, estas rutinas del BIOS dejan de ser accesibles. El kernel debe implementar sus propios drivers para cada dispositivo, empezando por la salida de video mediante escritura directa en la memoria VGA (`0xB8000`).

### El script de linker `link.ld`
 
```ld
SECTIONS {
    . = 0x7c00;            /* El BIOS carga el MBR en 0x7C00 */
    .text : {
        __start = .;
        *(.text)
        . = 0x1FE;         /* Posicionarse en el byte 510 */
        SHORT(0xAA55)      /* Boot signature */
    }
}
```
 
### Compilar y ejecutar
 
```bash
as -g -o src/main.o src/main.S
ld --oformat binary -o src/main.img -T src/link.ld src/main.o
qemu-system-x86_64 -hda src/main.img
```

![Hello SdeC](assets/mbr_hello_world.png)