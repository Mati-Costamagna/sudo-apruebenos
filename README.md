# Device Drivers

**Asignatura:** Sistemas de Computación  

**Profesores:** 
  - Jorge, Javier Alejandro
  - Solinas, Miguel Angel

**Estudiantes:** 
  - Costamagna, Matias
  - Davila Tomassi, Carlos Valentino
  - Sabena, Maria Pilar

**Link del repositorio:** https://github.com/Mati-Costamagna/sudo-apruebenos/tree/TP5

**Fecha:** Junio 2026

---

## Introducción

Un "driver" es aquel que conduce, administra y controla la entidad bajo su mando. En el contexto del software de sistemas, un **device driver** hace exactamente eso con un dispositivo: proporciona una abstracción del hardware y una interfaz, posiblemente estandarizada, para que el sistema operativo y las aplicaciones de usuario puedan interactuar con él sin conocer los detalles de su funcionamiento interno.

Es importante distinguir tres conceptos que suelen confundirse:

- **Device driver (software driver):** pieza de software que controla un dispositivo a través del sistema operativo. Es el foco de este trabajo.
- **Device controller:** dispositivo de hardware que gestiona otro dispositivo (ej. un controlador IDE, un controlador USB, un controlador SPI). Es hardware en sí mismo, y generalmente necesita su propio driver (denominado bus driver) para ser gestionado.
- **Bus driver:** driver del bus de comunicación subyacente; es la capa de software más baja del sistema operativo, sobre la cual se construyen los device drivers específicos.

En Linux, los device drivers se clasifican en tres grandes verticales según la naturaleza de su interfaz:

| Vertical | Orientación | Ejemplos |
|---|---|---|
| **Character (CDD)** | Byte a byte, secuencial | Puertos serie, audio, cámaras, GPIO |
| **Block** | Por bloques, acceso aleatorio | Discos, SSDs, particiones |
| **Network** | Por paquetes | Tarjetas de red, Wi-Fi |

El grupo mayoritario de drivers pertenece al vertical **Character**. Todo driver de dispositivo que no sea de almacenamiento ni de red es, en alguna forma, un Character Device Driver (CDD). La interfaz entre el espacio de usuario y el CDD se materializa a través de un **Character Device File (CDF)** en el directorio `/dev`: una aplicación abre ese archivo y realiza operaciones estándar (`open`, `read`, `write`, `close`, `ioctl`) que el Virtual File System (VFS) del kernel redirige a las funciones registradas por el driver.

El vínculo entre el CDF y el CDD no se basa en el nombre del archivo sino en un par de números (major:minor) que el kernel usa como índice. El número mayor identifica al driver responsable; el número menor distingue instancias dentro del mismo driver.

En este trabajo se construye un CDD desde cero, se lo conecta con una aplicación de espacio de usuario, y se estudia el flujo completo de datos entre hardware (real o simulado), kernel y userspace.

## Objetivos del trabajo

Este trabajo práctico cubre los siguientes temas:

- **Arquitectura de un CDD:** estructura de `file_operations`, registro de major/minor, creación automática del device node vía udev.
- **Ciclo de vida del driver:** `module_init()` / `module_exit()`, `alloc_chrdev_region()`, `class_create()`, `device_create()` y sus contrapartes de cleanup.
- **Transferencia de datos kernel ↔ userspace:** uso correcto de `copy_to_user()` y `copy_from_user()`; por qué no se puede usar `memcpy` directamente.
- **Timers en el kernel:** uso de `timer_list` para ejecutar código periódico en espacio de kernel (muestreo de señales a 1 Hz).
- **Sincronización en kernel:** protección de datos compartidos entre el timer callback y las `file_operations` mediante `spinlock`.
- **Aplicación de usuario:** lectura de un CDF, selección de señal vía `write()`, graficación en tiempo real con ejes correctamente etiquetados y reset al cambiar de señal.
- **Adaptación sin hardware dedicado:** estrategia para implementar y validar un driver de señales cuando no se dispone de Raspberry Pi física.

## Herramientas y entorno

```bash
# Dependencias del driver (kernel headers del kernel en uso)
sudo apt-get install build-essential linux-headers-$(uname -r)

# Dependencias de la aplicación de usuario
pip install matplotlib

# Repositorio de referencia de la cátedra
# https://gitlab.com/sistemas-de-computacion-unc/device-drivers/
```

El desarrollo se realiza sobre **Linux nativo x86-64**. Dado que el grupo no dispone de Raspberry Pi física, las dos señales externas se simulan mediante un timer de kernel que genera una onda senoidal (señal 0, temperatura) y un diente de sierra (señal 1, tensión). Esta estrategia permite verificar toda la cadena (driver, device node, aplicación y gráfico) sin hardware GPIO real. La variante con QEMU + `qemu-rpi-gpio` queda documentada como alternativa opcional.