[README.md](https://github.com/user-attachments/files/27329693/README.md)
# TP3 - Ejecución en Bare Metal (UEFI)

## Objetivo
Preparar un medio de arranque USB con una aplicación UEFI propia, bootear una computadora real (o emulada) y ejecutar el binario directamente sobre el hardware, sin sistema operativo intermedio.

## Requisitos
- PC con firmware UEFI (se deshabilita Secure Boot)
- Pendrive (mínimo 64 MB, formateado en FAT32)
- Linux (para preparar el USB)
- Aplicación UEFI compilada (`aplicacion.efi`)

## 1. Preparación del Pendrive

### 1.1 Formatear en FAT32

![Formateo del USB](img1_formateo_usb.png)

### 1.2 Compilación de la aplicación 
Se utilizo el archivo Makefile generado en la parte 2 para convertir "aplicacion" de ".c" a ".efi" y así ejecutarlo ahí.

![Compilación con make](img2_compilacion.png)

### 1.3 Navegación en la Shell UEFI
Tras desactivar el Secure Boot de esta Notebook HP, pudimos bootear desde el dispositivo USB.
![Shell UEFI en la laptop](img3_shell_uefi.png)
