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
# fork https://gitlab.com/sistemas-de-computacion-unc/kenel-modules.git
```

El desarrollo se realiza sobre Linux nativo. Algunos puntos de la consigna requieren hardware real para observar módulos de dispositivos cargados en el sistema (comparación entre integrantes del grupo).

---
