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

Salida de `dmesg` al cargar el módulo:

```
[ 3267.951241] mimodulo: loading out-of-tree module taints kernel.
[ 3267.951253] mimodulo: module verification failed: signature and/or required key missing - tainting kernel
[ 3267.951710] Modulo cargado en el kernel.
```

Las tres líneas reflejan el comportamiento esperado para un módulo sin firma:

1. El kernel advierte que es un módulo *out-of-tree* (no parte del árbol oficial del kernel) y marca el kernel como *tainted*.
2. La verificación de firma falla porque el módulo no está firmado con una clave en el keyring del sistema.
3. A pesar del taint, el módulo se carga y ejecuta `modulo_lin_init()`.

Verificación con `lsmod`:

```
mimodulo    12288  0
```

Para descargar el módulo:

```bash
sudo rmmod mimodulo
sudo dmesg | tail -3
```

Salida de `dmesg` al descargar:

```
[ 3355.058907] Modulo descargado del kernel.
```

Luego de `rmmod`, tanto `lsmod | grep mimodulo` como `cat /proc/modules | grep mimodulo` no devuelven resultado, confirmando que el módulo fue removido completamente del kernel.

### 2. Inspección con modinfo y /proc/modules

Se usó `modinfo` para comparar los metadatos del módulo propio con un módulo oficial del kernel.

**Nota sobre el formato `.ko.zst`:** A partir del kernel 5.19, Ubuntu distribuye los módulos comprimidos con Zstandard (`.ko.zst`) en lugar del tradicional `.ko`. El comando `modinfo /lib/modules/$(uname -r)/kernel/crypto/des_generic.ko` falla porque el archivo no existe sin extensión; la ruta correcta es con `.ko.zst`. `modinfo` soporta este formato de forma transparente.

```bash
modinfo mimodulo.ko
modinfo /lib/modules/$(uname -r)/kernel/crypto/des_generic.ko.zst
```

Salida de `modinfo mimodulo.ko`:

```
filename:       /home/matias/.../kenel-modules/part1/module/mimodulo.ko
author:         Catedra de SdeC
description:    Primer modulo ejemplo
license:        GPL
srcversion:     C6390D617B2101FB1B600A9
depends:        
name:           mimodulo
retpoline:      Y
vermagic:       6.17.0-14-generic SMP preempt mod_unload modversions
```

Salida de `modinfo des_generic.ko.zst`:

```
filename:       /lib/modules/6.17.0-14-generic/kernel/crypto/des_generic.ko.zst
alias:          crypto-des3_ede-generic
alias:          des3_ede-generic
alias:          crypto-des-generic
alias:          des-generic
alias:          crypto-des
alias:          des
author:         Dag Arne Osvik <da@osvik.no>
description:    DES & Triple DES EDE Cipher Algorithms
license:        GPL
srcversion:     A4E91C81A384FB1FA6D530F
depends:        libdes
intree:         Y
name:           des_generic
retpoline:      Y
vermagic:       6.17.0-14-generic SMP preempt mod_unload modversions
sig_id:         PKCS#7
signer:         Build time autogenerated kernel key
sig_key:        56:AF:AB:A9:4B:3E:22:5B:8C:F1:E3:65:E0:45:5D:EB:1E:EF:C2:58
sig_hashalgo:   sha512
signature:      09:D8:05:15:89:CA:F6:7D:...
```

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




