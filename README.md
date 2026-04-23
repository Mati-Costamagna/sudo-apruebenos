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

Los procesadores x86 mantienen compatibilidad con sus antecesores mediante un proceso de evolucion durante el arranque. Al energizarse, todo CPU x86 comienza en **modo real** para garantizar compatibilidad hacia atras con el 8086 original, comportandose de manera primitiva con acceso a solo 1 MB de memoria y sin ningún mecanismo de proteccion.
 
El **modo protegido** es el primer salto evolutivo significativo, introducido con el 80286. Sus caracteristicas principales son:
 
- **Proteccion de memoria**: los programas no pueden acceder a zonas de memoria que no les corresponden.
- **Memoria virtual**: soporte por hardware para memoria que excede la RAM física.
- **Multitarea**: conmutacion de tareas gestionada por hardware.
- **Niveles de privilegio (rings)**: separacion entre codigo del kernel y codigo de usuario.
El objetivo de este TP es comprender y ejecutar la transicion desde modo real a modo protegido en un procesador x86, utilizando QEMU como entorno de virtualizacion.

---
