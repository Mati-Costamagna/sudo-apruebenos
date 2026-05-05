# TP3 - Ejecución en Bare Metal (UEFI)

## Objetivo
El objetivo de esta tercera parte es preparar un medio de arranque USB con una aplicación UEFI propia, bootear una computadora real (o emulada) y ejecutar el binario directamente sobre el hardware, sin sistema operativo intermedio. En este caso utilizaremos una Notebook HP, con un firmware UEFI.

## Requisitos
- PC con firmware UEFI (se deshabilita Secure Boot)
- Pendrive (mínimo 64 MB, formateado en FAT32)
- Linux (para preparar el USB)
- Aplicación UEFI compilada (`aplicacion.efi`)

## 1. Preparación del Pendrive

### 1.1 Formatear en FAT32
Para cumplir con el requerimiento de la UEFI, se formatea en FAT32, montamos el pendrive y creamos la estructura estandarizada de directorios, dentro de la cual se encuentra la UEFI Shell oficial de TianoCore : https://github.com/tianocore/edk2/raw/UDK2018/ShellBinPkg/UefiShell/X64/Shell.efi -O /mnt/EFI/BOOT/BOOTX64.EFI

![Formateo del USB](assets/img1_formateo_usb.png)

### 1.2 Compilación de la aplicación 
Clonamos el repositorio, en cuya parte 2 encontramos "aplicacion.c" y el Makefile, el cual utilizamos para convertir "aplicacion" de ".c" a ".efi", resumiendo los pasos intermedios antes desarrollados en la parte 2.

![Compilación con make](assets/img2_compilacion.png)

Una vez incluido dentro de la carpeta del pendrive, este está listo para ejecutarse Bare Metal (con la configuracion de Boot adecuada).

### 1.3 Navegación en la Shell UEFI
Tras reiniciar el ordenador de prueba y abrir las configuraciones de boot, desactivamos Secure Boot. Tras desactivar el Secure Boot de esta Notebook HP, pudimos bootear desde el dispositivo USB. Al ingresar en la Shell, podemos ver varias carpetas como FS0:, FS1:, BLK0:, BLK2: que son las distintas carpetas del USB, dentro de FS0: hacemos "ls" y vemos aplicacion.efi.
![Shell UEFI en la laptop](assets/img3_shell_uefi.png)
Al ejecutar la aplicación con el binario original, tanto en la PC de escritorio como en la HP, la pantalla se congelaba y había que reiniciar. El mismo comportamiento se reproducía en QEMU.

### 1.4 Corrección y verificación en QEMU

El problema era un **choque de ABI**: el código original llamaba a `SystemTable->ConOut->OutputString` directamente desde C, que usa la convención System V AMD64 (Linux), mientras que UEFI espera la Microsoft x64 ABI. La solución fue reemplazar esas llamadas por `Print()` de `efilib.h`, que internamente usa `uefi_call_wrapper` para ajustar la convención.

Además, sin una pausa al final, UEFI devolvía el control al firmware inmediatamente y la pantalla quedaba en un estado indefinido. Se agregó un bucle de espera de teclado con `ConIn->ReadKeyStroke` vía `uefi_call_wrapper`.

Con el código corregido y recompilado, la ejecución en QEMU funciona correctamente:

![Ejecución en QEMU](assets/img4_qemu_ejecucion.png)

La captura muestra la UEFI Interactive Shell v2.2 (EDK II / OVMF, Ubuntu distribution), el volumen `FS0:` con `aplicacion.efi`, y la salida completa de la aplicación:

```
Iniciando analisis de seguridad...
Breakpoint estatico validado en memoria.

Presiona cualquier tecla para finalizar el analisis...
```

El binario ejecutado es el mismo `.efi` generado en la Parte 2; la diferencia es únicamente el código fuente corregido.
