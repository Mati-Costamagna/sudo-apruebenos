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

El modo protegido resuelve exactamente ese problema. Introducido con el 80286, fue el primer intento de agregar hardware especifico para aislar procesos entre sí y del sistema operativo. Sin embargo, la implementacion del 286 era incompleta: una vez en modo protegido, no habia forma de volver a modo real sin resetear el procesador, lo que lo hizo poco practico. Fue el 80386 quien consolido el modelo que usamos hasta hoy, agregando paginacion, modos de 32 bits completos y la posibilidad de retornar a modo real mediante software.

Sus características principales son:

- Proteccion de memoria: los programas no pueden acceder a zonas de memoria que no les corresponden. El hardware verifica cada acceso.
- Memoria virtual: soporte por hardware para memoria que excede la RAM fisica, mediante paginacion.
- Multitarea: conmutacion de tareas gestionada por hardware con contextos separados.
- Niveles de privilegio (rings): separacion entre codigo del kernel y codigo de usuario, con el hardware actuando como arbitro.

El objetivo de este TP es comprender y ejecutar la transicion desde modo real a modo protegido en un procesador x86, utilizando QEMU como entorno de virtualizacion.

---
