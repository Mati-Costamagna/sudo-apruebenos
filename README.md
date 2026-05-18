# Módulos de Kernel de Linux

**Asignatura:** Sistemas de Computación  

**Profesores:** 
  - Jorge, Javier Alejandro
  - Solinas, Miguel Angel

**Estudiantes:** 
  - Costamagna, Matias
  - Davila Tomassi, Carlos Valentino
  - Sabena, Maria Pilar

**Link del repositorio:** https://github.com/Mati-Costamagna/sudo-apruebenos/tree/TP4

**Fecha:** Mayo 2026

---

## Introducción

Un módulo de kernel es un fragmento de código que puede cargarse y descargarse en el kernel de Linux de forma dinámica, sin necesidad de reiniciar el sistema. Esta capacidad permite extender la funcionalidad del kernel en tiempo de ejecución: los drivers de dispositivos, los sistemas de archivos y los protocolos de red son ejemplos típicos de funcionalidad implementada como módulos.

La alternativa a los módulos sería un kernel monolítico donde toda funcionalidad se compila estáticamente en la imagen del kernel. Esto implica que cualquier nueva característica exigiría recompilar y reiniciar el sistema, lo cual resulta impráctico en entornos de producción.

A diferencia de un programa de usuario, un módulo se ejecuta en el **espacio del kernel** (ring 0), con acceso directo al hardware y a las estructuras internas del sistema operativo. Esta diferencia tiene implicancias profundas: un puntero inválido en espacio de usuario genera un `segmentation fault` y el proceso termina; el mismo error en espacio de kernel puede corromper el sistema entero y forzar un reinicio. El kernel no tiene la red de seguridad que el SO le ofrece a los procesos de usuario.

## Objetivos del trabajo

Este trabajo práctico cubre los siguientes temas:

- **Ciclo de vida de un módulo:** compilación, carga con `insmod`, descarga con `rmmod`, inspección con `lsmod`, `modinfo` y `/proc/modules`.
- **Espacio de usuario vs. espacio de kernel:** diferencias en funciones disponibles, manejo de errores y acceso a recursos.
- **Drivers y el directorio `/dev`:** relación entre módulos de kernel y los archivos de dispositivo que exponen al espacio de usuario.
- **Llamadas al sistema:** uso de `strace` para observar la interfaz entre un proceso y el kernel.
- **Firma de módulos y Secure Boot:** mecanismos para restringir la carga de módulos no firmados y sus implicancias en la seguridad del arranque.
- **`checkinstall`:** empaquetado de software compilado manualmente en paquetes del sistema (`.deb`).

## Herramientas y entorno

```bash
# Dependencias
sudo apt-get install build-essential checkinstall kernel-package linux-source

# Repositorio base de la cátedra
# https://gitlab.com/sistemas-de-computacion-unc/kenel-modules.git
```

El desarrollo se realiza sobre Linux nativo. Algunos puntos de la consigna requieren hardware real para observar módulos de dispositivos cargados en el sistema (comparación entre integrantes del grupo).

---

## Desarrollo

### 1. Setup y primer módulo

Se clonó el repositorio de la cátedra y se compiló el módulo de ejemplo:

```bash
git clone https://gitlab.com/sistemas-de-computacion-unc/kenel-modules.git
cd kenel-modules/part1/module
make
```

La compilación genera `mimodulo.ko`. A continuación se cargó en el kernel:

```bash
sudo insmod mimodulo.ko
sudo dmesg | tail -10
lsmod | grep mimodulo
```

Salida de `dmesg` al cargar el módulo y verificación con `lsmod`:

![Salida de insmod y lsmod](assets/insmod.png)

Las tres líneas reflejan el comportamiento esperado para un módulo sin firma:

1. El kernel advierte que es un módulo *out-of-tree* (no parte del árbol oficial del kernel) y marca el kernel como *tainted*.
2. La verificación de firma falla porque el módulo no está firmado con una clave en el keyring del sistema.
3. A pesar del taint, el módulo se carga y ejecuta `modulo_lin_init()`.

Para descargar el módulo:

```bash
sudo rmmod mimodulo
sudo dmesg | tail -3
```

Salida de `dmesg` al descargar:

![Salida de rmmod](assets/rmmod.png)

Luego de `rmmod`, tanto `lsmod | grep mimodulo` como `cat /proc/modules | grep mimodulo` no devuelven resultado, confirmando que el módulo fue removido completamente del kernel.

### 2. Inspección con modinfo y /proc/modules

Se usó `modinfo` para comparar los metadatos del módulo propio con un módulo oficial del kernel.

**Nota sobre el formato `.ko.zst`:** A partir del kernel 5.19, Ubuntu distribuye los módulos comprimidos con Zstandard (`.ko.zst`) en lugar del tradicional `.ko`. El comando `modinfo /lib/modules/$(uname -r)/kernel/crypto/des_generic.ko` falla porque el archivo no existe sin extensión; la ruta correcta es con `.ko.zst`. `modinfo` soporta este formato de forma transparente.

```bash
modinfo mimodulo.ko
modinfo /lib/modules/$(uname -r)/kernel/crypto/des_generic.ko.zst
```

Salida de `modinfo mimodulo.ko`:

![Salida de modinfo mimodulo](assets/modinfo.png)

Salida de `modinfo des_generic.ko.zst`:

![Salida de modinfo des_generic](assets/modinfo_des_generic.png)

**Comparación de campos:**

| Campo | `mimodulo.ko` | `des_generic.ko.zst` |
|---|---|---|
| `author` | Catedra de SdeC | Dag Arne Osvik |
| `description` | Primer modulo ejemplo | DES & Triple DES EDE Cipher Algorithms |
| `depends` | *(ninguno)* | `libdes` |
| `intree` | *(ausente)* | `Y` |
| `alias` | *(ninguno)* | 8 aliases crypto |
| `sig_id` / `sig_key` | *(ausente — sin firma)* | PKCS#7 / clave autogenerada en build |
| `vermagic` | idéntico en ambos | idéntico en ambos |

Las diferencias más relevantes son:

- **`intree: Y`** indica que `des_generic` forma parte del árbol oficial del kernel; `mimodulo` no tiene este campo porque es un módulo externo (*out-of-tree*).
- **Firma (`sig_*`)**: `des_generic` está firmado con una clave autogenerada durante la compilación del kernel de Ubuntu. `mimodulo` carece de firma, lo que produjo el mensaje de *taint* al cargarlo.
- **`alias`**: los módulos del kernel pueden registrar aliases para que `modprobe` los cargue automáticamente al detectar hardware compatible. Un módulo simple de ejemplo no necesita aliases.
- **`vermagic`**: es idéntico en ambos porque ambos fueron compilados para el mismo kernel (`6.17.0-14-generic`). Si difirieran, el kernel rechazaría la carga.

### 3. Módulos cargados por integrante del grupo

Cada integrante exportó la lista de módulos activos en su sistema:

```bash
lsmod > lsmod_<nombre>.txt
```

Los archivos individuales están disponibles en el repositorio (`lsmod_costamagna.txt`, `lsmod_davila.txt`, `lsmod_sabena.txt`).

**Módulos cargados vs. disponibles (Costamagna):**

| Métrica | Valor |
|---|---|
| Módulos actualmente cargados | 197 |
| Módulos disponibles en `/lib/modules/` | 6769 |
| Porcentaje cargado | ~2.9 % |

La diferencia es significativa: el kernel carga únicamente los módulos necesarios para el hardware detectado y los servicios activos. Los 6569 módulos restantes están disponibles en disco pero inactivos; `modprobe` o `udev` los cargarán automáticamente si el hardware correspondiente es detectado o si un proceso los solicita.

> **Pendiente:** agregar salidas de `lsmod_davila.txt` con el diff entre integrantes una vez que los demás miembros del grupo ejecuten el comando.

**Módulos cargados vs. disponibles (Sabena)**

| Métrica | Valor |
|---|---|
| Módulos actualmente cargados | 163 |
| Módulos disponibles en `/lib/modules/` | 6377 |
| Porcentaje cargado | ~2.6 % |

Solo se carga alrededor del 2.6 % de los módulos disponibles en disco. El resto permanece inactivo hasta que `udev` detecte el hardware correspondiente o algún proceso lo solicite explícitamente via `modprobe`.


**¿Qué pasa cuando un driver no está disponible?**

Cuando se conecta un dispositivo y el módulo que lo gestiona no existe en el sistema, el kernel emite en `dmesg` un mensaje similar a:

```
usbcore: registered new interface driver <nombre>
<nombre>: probe failed with error -22
```

O bien `udev` intenta cargarlo con `modprobe` y falla silenciosamente. El dispositivo queda sin funcionalidad (no aparece en `/dev` o aparece pero sin driver asociado). El sistema no se interrumpe: simplemente ese hardware queda inoperativo.

**Comparación de módulos entre integrantes:**

#### Comparación de módulos entre integrantes

```bash
diff lsmod_costamagna.txt lsmod_sabena.txt lsmod_davila.txt
```

| Aspecto | Costamagna | Sabena | Davila |
|---|---|---|---|
| **GPU** | AMD (`amdgpu`) | Intel integrado (`i915`) | — |
| **Audio** | AMD (`snd_sof_amd_acp`) | Intel Tiger Lake (`snd_sof_intel_hda_common`) | — |
| **WiFi** | Intel (`iwlwifi`) | MediaTek (`mt7921e`) | — |
| **Virtualización** | AMD-V (`kvm_amd`) | VT-x (`kvm_intel`) | — |
| **Contenedores** | Docker activo | Sin contenedores | — |

> **Pendiente:** completar con los módulos de Davila.

Lo que se ve en la tabla tiene sentido: cada sistema carga exactamente los drivers del hardware que tiene instalado. La GPU, el chip de audio y la placa WiFi son distintos en cada máquina, entonces los módulos también lo son. Lo único que coincide entre los tres sistemas son los subsistemas genéricos como USB, Bluetooth o la cámara, que funcionan igual en cualquier equipo.

### 4. Hardware real: hwinfo

Cada integrante instaló y ejecutó la herramienta de diagnóstico de hardware `hwinfo` para relevar los componentes físicos del sistema y comprender de manera empírica qué módulos del kernel se necesitan activar para darles soporte.

Los comandos utilizados en la terminal para instalar la utilidad y exportar el informe resumido fueron:

```bash
sudo apt install hwinfo
hwinfo --short > hwinfo_apellido.txt
```


Los reportes completos se encuentran adjuntos en los siguientes archivos del repositorio:
- [Reporte de Matias Costamagna](hwinfo_costamagna.txt)
- [Reporte de Carlos Valentino Davila](hwinfo_davila.txt) (Pendiente de subir)
- [Reporte de Pilar Sabena](hwinfo_sabena.txt)

**Breve descripción del hardware detectado (Matias Costamagna):**

El sistema de Costamagna está basado en un procesador **AMD Ryzen 7 4700U** (arquitectura Renoir, 8 núcleos) con gráficos integrados **ATI Renoir** gestionados por el driver `amdgpu`. El almacenamiento es un SSD **Samsung NVMe** (`/dev/nvme0n1`), soportado por el driver `nvme`. La conectividad inalámbrica y Bluetooth provienen de una placa **Intel Wi-Fi 6 AX200**, que utiliza el módulo `iwlwifi`. El audio es manejado por `snd_hda_intel` a través del controlador AMD Family 17h HD Audio. Se detectaron también interfaces de red virtuales correspondientes a Docker (`docker0`) y libvirt (`virbr0`), lo que confirma los módulos de virtualización y contenedores (`kvm_amd`, `bridge`) visibles en la Sección 3.

**Breve descripción del hardware detectado (Pilar Sabena):**
Al inspeccionar el archivo `hwinfo_sabena.txt`, se observa que el kernel de Linux interactúa directamente con una arquitectura basada en **Intel** (procesador y gráficos integrados a través del driver `i915`), componentes de almacenamiento masivo **NVMe**, y adaptadores de red inalámbrica gestionados dinámicamente por módulos del kernel. Esto ratifica el análisis de la Sección 3, donde los módulos cargados en memoria responden estrictamente a este inventario de componentes físicos.

### 5. ¿Qué diferencia existe entre un módulo y un programa?

La diferencia más básica está en cómo empiezan, terminan y qué pueden usar.

Un programa normal arranca desde `main()`, hace lo suyo y cuando termina le devuelve el control al sistema operativo. Un módulo no tiene `main()`: tiene `module_init()` para cuando se carga y `module_exit()` para cuando se descarga. Entre medio no "corre" de forma continua — queda registrado en el kernel esperando que algo lo invoque.

La otra diferencia importante es qué funciones tienen disponibles. Un programa puede llamar a cualquier función de la biblioteca estándar de C (`printf`, `malloc`, etc.), que internamente hacen llamadas al sistema. Un módulo no puede usar nada de eso: solo puede llamar a las funciones que el propio kernel exporta. Esos símbolos están listados en `/proc/kallsyms`.

Y la consecuencia más importante de todo esto: un programa corre en espacio de usuario, así que si falla, el SO lo mata y el resto del sistema sigue andando. Un módulo corre en espacio de kernel, al mismo nivel que el propio SO. Si un módulo hace algo mal — un puntero inválido, por ejemplo — no hay nadie que lo atrape: el kernel puede caerse entero.

### 6. Llamadas al sistema de un hello world

Se compiló y ejecutó un programa simple con `strace` para observar qué syscalls realiza:

```bash
gcc -Wall -o hello hello.c
strace ./hello
strace -c ./hello
```

La salida completa de cada integrante está en `strace_apellido.txt` respectivamente en el repositorio. 

**Análisis de la salida (Sabena)**

Lo primero que llama la atención es la cantidad de syscalls que genera un programa tan simple: 35 llamadas en total para imprimir una sola línea. Esto se explica porque la mayor parte del trabajo no es el `printf` en sí, sino la inicialización del proceso y la carga de la libc.

El flujo que se puede seguir en la salida es este:

- `execve` — el kernel arranca el proceso cargando el binario
- `openat` + `mmap` — el linker dinámico busca y mapea en memoria la librería `libc.so.6`
- `brk` + `mmap` — se reserva memoria para el heap y otros segmentos
- `write(1, "Hola Sistemas de Computacion!\n", 30)` — acá es donde ocurre el `printf` real: una sola llamada a `write` sobre el descriptor 1 (stdout)
- `exit_group(0)` — el proceso termina

El punto importante es que `printf` no es una syscall: es una función de la libc que internamente termina llamando a `write`, que sí lo es. Todo lo que un programa hace "visible" al SO pasa por esta interfaz de syscalls, y `strace` permite verla completa.

Los reportes completos se encuentran en el repositorio:
- [strace Matias Costamagna](strace_costamagna.txt) (Pendiente de subir)
- [strace Carlos Valentino Davila](strace_davila.txt) (Pendiente de subir)
- [strace Maria Pilar Sabena](strace_sabena.txt)

### 7. ¿Qué es un segmentation fault y cómo lo manejan el kernel y un programa?

Un segmentation fault ocurre cuando un proceso intenta acceder a una dirección de memoria que no le corresponde: leer o escribir fuera de su espacio asignado, desreferenciar un puntero nulo, etc.

Cuando eso pasa en un programa de usuario, el hardware genera una excepción (page fault) que el kernel intercepta. El kernel determina que el acceso es inválido, le manda la señal `SIGSEGV` al proceso y lo termina. El resto del sistema no se ve afectado.

En un módulo de kernel la historia es distinta. No hay nadie por encima que pueda interceptar el error y contenerlo. Un acceso de memoria inválido en espacio de kernel genera un **kernel panic** o un **oops** — el sistema puede quedar inestable o directamente reiniciarse. No hay red de seguridad.