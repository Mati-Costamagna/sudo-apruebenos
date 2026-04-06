# Informe Fase 1: Integración Python + C

**Asignatura:** Sistemas de Computación  

**Profesores:** 
  - Jorge, Javier Alejandro
  - Solinas, Miguel Angel

**Estudiantes:** 
  - Costamagna, Matias
  - Davila Tomassi, Carlos Valentino
  - Sabena, Maria Pilar

**Fecha:** Abril 2026

**Link al repositorio del TP:** https://github.com/Mati-Costamagna/sudo-apruebenos/tree/TP2

---

## Introducción

Esta primera fase del proyecto establece las bases de la **interoperabilidad entre Python y C** mediante el uso de **ctypes** (Foreign Function Interface). El objetivo es demostrar cómo un lenguaje interpretado de alto nivel puede comunicarse con código compilado nativo sin complicaciones de bajo nivel.

---

## Objetivos de la Fase 1

### Objetivo General
Implementar la comunicación básica entre Python y C para procesar datos del Índice GINI obtenidos de la API del Banco Mundial.

### Objetivos Específicos
1. **Consumir datos externos** mediante API REST (Banco Mundial)
2. **Integrar C desde Python** usando FFI (ctypes)
3. **Procesar datos numéricos** en C puro (sin Assembly)
4. **Establecer la base arquitectónica** para la Fase 2

---


## Flujo de Datos

![Diagrama de Flujo Fase 1](diagrama_fase1.svg)

### Descripción del Flujo

1. **Solicitud HTTP**: Python solicita datos GINI a la API del Banco Mundial
2. **Parseo JSON**: Los datos JSON se convierten en estructuras Python
3. **Carga de Librería**: ctypes carga `libgini.so` (librería compartida C)
4. **Marshalling**: ctypes convierte tipos Python a tipos C
5. **Ejecución C**: Las funciones C procesan los datos
6. **Retorno**: Los resultados vuelven a Python como tipos nativos

---

## Componentes del Sistema

### 1. Capa Superior: Python (`main.py`)

```python
# fase1/main.py
import requests
import ctypes
import json

# URL de la API del Banco Mundial
WORLDBANK_API_URL = "http://api.worldbank.org/v2/country/ARG/indicator/SI.POV.GINI?format=json&per_page=100"

def obtener_datos_gini():
    """Obtiene datos GINI desde la API del Banco Mundial"""
    response = requests.get(WORLDBANK_API_URL)
    data = response.json()
    
    # El segundo elemento contiene los datos
    datos = data[1]
    
    # Filtrar solo los valores no nulos
    return [d for d in datos if d['value'] is not None]

def main():
    # 1. Obtener datos de API
    datos_gini = obtener_datos_gini()
    
    # 2. Cargar librería C
    lib = ctypes.CDLL('./libgini.so')
    
    # 3. Configurar tipos de argumentos y retorno
    lib.float_to_int.argtypes = [ctypes.c_float]
    lib.float_to_int.restype = ctypes.c_int
    
    lib.sumar_uno.argtypes = [ctypes.c_int]
    lib.sumar_uno.restype = ctypes.c_int
    
    # 4. Procesar cada dato
    print("\nResultados: Argentina (Últimos 5 años) - FASE 1 (C Pura)")
    print("AÑO    | GINI ORIGINAL   | ENTERO (C) | SUMA +1 (C)")
    print("-------+-----------------+------------+------------")
    
    for dato in datos_gini[:5]:
        año = dato['date']
        gini_original = dato['value']
        
        # Llamar funciones C
        gini_int = lib.float_to_int(gini_original)
        gini_plus_one = lib.sumar_uno(gini_int)
        
        print(f"{año}   | {gini_original:<15.2f} | {gini_int:<10} | {gini_plus_one}")

if __name__ == "__main__":
    main()
```

---

### 2. Capa Intermedia: ctypes (FFI)

**ctypes** es el módulo de la biblioteca estándar de Python que permite:

- **Cargar librerías compartidas** (`.so` en Linux, `.dll` en Windows)
- **Declarar tipos de datos C** desde Python
- **Llamar funciones C** como si fueran funciones Python
- **Convertir automáticamente** entre tipos Python y C (marshalling)

#### Proceso de Marshalling

Cuando Python llama a una función C:

```python
gini_int = lib.float_to_int(42.3)
```

ctypes realiza los siguientes pasos:

1. **Extrae** el valor numérico del objeto Python `float`
2. **Convierte** el tipo: `Python float → ctypes.c_float`
3. **Prepara** los registros según la convención de llamadas (System V AMD64 ABI)
4. **Invoca** la función en la librería compartida
5. **Recibe** el valor de retorno desde el registro `%eax`
6. **Convierte** de vuelta: `C int → Python int`

#### Declaración de Tipos

```python
# Especificar tipos de argumentos
lib.float_to_int.argtypes = [ctypes.c_float]

# Especificar tipo de retorno
lib.float_to_int.restype = ctypes.c_int
```

Esto es **crítico** porque ctypes necesita saber:
- Cuántos bytes ocupan los argumentos
- En qué registros o posiciones del stack colocarlos
- Cómo interpretar el valor de retorno

---

### 3. Capa Inferior: C (`gini.c`)

```c
// fase1/gini.c - Implementación en C Puro

/**
 * Convierte un número de punto flotante a entero (truncando)
 * @param value Valor float a convertir
 * @return Parte entera del valor (truncado)
 */
int float_to_int(float value) {
    return (int)value;  // Conversión simple en C
}

/**
 * Suma 1 a un número entero
 * @param value Valor entero
 * @return value + 1
 */
int sumar_uno(int value) {
    return value + 1;
}
```

#### Compilación

```bash
# Compilar como librería compartida
gcc -shared -fPIC -o libgini.so gini.c

# -shared: Crear librería compartida (.so)
# -fPIC: Position Independent Code (requerido para librerías compartidas)
# -o libgini.so: Nombre del archivo de salida
```

---

## Makefile

```makefile
# fase1/Makefile

CC = gcc
CFLAGS = -Wall -Wextra -fPIC
LDFLAGS = -shared

TARGET = libgini.so
SRC = gini.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run: all
	python3 main.py

.PHONY: all clean run
```

---

## Casos de Prueba

### Tabla de Pruebas - Fase 1

| # | Entrada (float) | Esperado (int) | Actual (int) | Suma +1 |
|---|-----------------|----------------|--------------|---------|
| 1 | 42.3 | 42 | 42 | 43 |
| 2 | 42.99 | 42 | 42 | 43 |
| 3 | 3.14159 | 3 | 3 | 4 |
| 4 | 0.5 | 0 | 0 | 1 |
| 5 | -15.7 | -15 | -15 | -14 |
| 6 | 0.0 | 0 | 0 | 1 |
| 7 | 99.99 | 99 | 99 | 100 |

### Información sobre Truncamiento

La conversión `(int)value` en C trunca hacia **cero**:
- `42.99` → `42` (no 43, aunque esté más cerca)
- `-15.7` → `-15` (no -16)
- `0.5` → `0` (no 1)

Este es el comportamiento esperado de **truncamiento**, no redondeo.

---

## Ejecución de la Fase 1

### Compilar y Ejecutar

```bash
$ cd fase1
$ make all
gcc -Wall -Wextra -fPIC -shared -o libgini.so gini.c

$ python3 main.py

Resultados: Argentina (Últimos 5 años) - FASE 1 (C Pura)
AÑO    | GINI ORIGINAL   | ENTERO (C) | SUMA +1 (C)
-------+-----------------+------------+------------
2020   | 42.30           | 42         | 43
2019   | 41.50           | 41         | 42
2018   | 41.20           | 41         | 42
2017   | 42.40           | 42         | 43
2016   | 42.70           | 42         | 43
```

---

## Análisis de Performance - Fase 1

### Benchmark: Python vs C

```python
# benchmark_fase1.py
import ctypes
import time

lib = ctypes.CDLL('./libgini.so')
lib.float_to_int.argtypes = [ctypes.c_float]
lib.float_to_int.restype = ctypes.c_int

# Prueba 1: Python nativo
start = time.perf_counter()
for _ in range(100000):
    result = int(42.3)
end = time.perf_counter()
tiempo_python = (end - start) / 100000 * 1_000_000  # μs

# Prueba 2: C mediante ctypes
start = time.perf_counter()
for _ in range(100000):
    result = lib.float_to_int(42.3)
end = time.perf_counter()
tiempo_c = (end - start) / 100000 * 1_000_000  # μs

print(f"Python int():        {tiempo_python:.3f} μs")
print(f"C via ctypes:        {tiempo_c:.3f} μs")
print(f"Overhead de ctypes:  {tiempo_c - tiempo_python:.3f} μs")
```

### Resultados Típicos

```
Implementación         Tiempo promedio    
──────────────────────────────────────────
Python int()           0.025 μs           
C via ctypes           1.200 μs           
Overhead de ctypes     ~1.175 μs          
```

### Análisis

1. **Python es ~48x más rápido** para esta operación trivial
2. El overhead de ctypes (~1.2 μs) incluye:
   - Marshalling de argumentos
   - Llamada a función compartida
   - Unmarshalling del retorno
3. **Cuándo usar C**: Cuando la lógica es suficientemente compleja para que el tiempo de ejecución supere el overhead de FFI

---

## Conclusiones de la Fase 1

### Aprendizajes Clave

1. **Interoperabilidad Python-C**
   - ctypes permite llamar C desde Python sin extensiones nativas
   - El marshalling es automático pero tiene overhead
   - La declaración de tipos (`argtypes`, `restype`) es esencial

2. **Librerías Compartidas**
   - Las librerías `.so` contienen código compilado reutilizable
   - `-fPIC` es necesario para código independiente de posición
   - Las funciones C son accesibles mediante símbolos exportados

3. **Performance**
   - El overhead de FFI (~1 μs) es significativo para operaciones triviales
   - Python nativo es más rápido cuando la lógica es simple
   - C es útil cuando el cálculo justifica el overhead

4. **Base para Fase 2**
   - Esta arquitectura permite agregar Assembly sin cambiar Python
   - El flujo de datos está establecido y probado
   - Los tipos y conversiones están validados

---

## Próximos Pasos (Fase 2)

En la siguiente fase del proyecto:

1. **Reemplazar la lógica C** por rutinas en Assembly x86-64
2. **Implementar Stack Frame** para comprender la convención de llamadas
3. **Forzar parámetros al stack** usando "dummy arguments"
4. **Inspeccionar memoria** con GDB para validar el layout del stack
5. **Comparar performance** entre C puro y C+Assembly