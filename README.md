# TP1 — El Rendimiento de las Computadoras

## Parte 1 — Lista de benchmarks por tarea diaria

La mejor forma de evaluar el rendimiento de un sistema es medir el tiempo que tarda en ejecutar los programas que el usuario realmente utiliza. A continuación se presenta una tabla de doble entrada con tareas cotidianas y el benchmark que mejor las representa.

| Tarea diaria | Benchmark representativo | Tipo | Fuente |
|---|---|---|---|
| Compilar proyectos de software | Timed Linux Kernel Compilation (`pts/build-linux-kernel`) | Kernel | openbenchmarking.org |
| Cálculo numérico / simulaciones | LINPACK / HPL (mide GFLOPS) | Sintético | top500.org / pts/linpack |
| Procesamiento de imágenes y video | HandBrake H.265 encoding (`pts/encode-video`) | Kernel | openbenchmarking.org |
| Desarrollo web / JavaScript | Speedometer 3.0 (simula apps web modernas) | Real | browserbench.org |
| Trabajo con bases de datos | pgbench / sysbench (transacciones por segundo) | Kernel | pts/pgbench |
| Machine learning / entrenamiento | MLPerf Training (tiempo hasta accuracy objetivo) | Real | mlcommons.org |
| Uso general de escritorio | PCMark 10 / Geekbench 6 | Sintético | futuremark.com |
| Compresión y archivado | 7-Zip benchmark (`pts/compress-7zip`) | Kernel | openbenchmarking.org |

**Tipos de benchmark:**
- **Sintético:** programa pequeño y enfocado en un tipo de carga específico.
- **Kernel:** extracto representativo de una carga de trabajo real.
- **Real:** programa real ejecutado con un patrón de uso típico del usuario.

---

## Parte 2 — Rendimiento en compilación del kernel Linux

**Benchmark utilizado:** `pts/build-linux-kernel`  
**Carga de trabajo:** compilación de Linux con configuración `defconfig`, arquitectura x86_64.  
**Métrica:** segundos totales de compilación — menor es mejor.  
**Fuente:** OpenBenchmarking.org (datos verificados con capturas del sitio)

### Especificaciones de los procesadores (fuente: OpenBenchmarking.org)

| Procesador | Núcleos | Hilos | CPU Clock | Core Family | Año | Percentil general |
|---|---|---|---|---|---|---|
| AMD Ryzen 9 5900X 12-Core | 12 | 24 | 3.7 GHz | Zen 3 | 2020 | 55° |
| Intel Core i5-13600K | 14 | 20 | 5.1 GHz | Raptor Lake | 2022 | 57° |
| AMD Ryzen 9 7950X 16-Core | 16 | 32 | 4.5 GHz | Zen 4 | 2022 | 72° |

### Rendimiento general — Media geométrica de todos los benchmarks

La media geométrica resume el rendimiento relativo a través de los 348 benchmarks en común registrados en OpenBenchmarking.org. Mayor es mejor.

| Procesador | Media geométrica | Ranking |
|---|---|---|
| AMD Ryzen 9 7950X 16-Core | **94.41** | 1° |
| Intel Core i5-13600K | **67.41** | 2° |
| AMD Ryzen 9 5900X 12-Core | **59.63** | 3° |

> El i5-13600K supera al Ryzen 9 5900X en rendimiento general a pesar de tener solo 14 núcleos contra 12, gracias a su arquitectura más nueva (Raptor Lake 2022 vs Zen 3 2020) y mayor frecuencia de clock (5.1 GHz vs 3.7 GHz).

### Rendimiento en compilación del kernel Linux

| Procesador | Tiempo aprox. (s) | Percentil compilación |
|---|---|---|
| Intel Core i5-13600K | ~83 s | 56° |
| AMD Ryzen 9 5900X 12-Core | ~76 s | 56° |
| AMD Ryzen 9 7950X 16-Core | ~54 s | ~70° |

> En este benchmark específico el 5900X supera levemente al i5-13600K gracias a su mayor cantidad de hilos (24 vs 20), lo que favorece la compilación paralela con `make -j`. Esto ilustra que el rendimiento es relativo a la carga de trabajo.

---

## Parte 3 — Speedup y eficiencia del Ryzen 9 7950X

### Definiciones

El speedup es la razón entre el rendimiento del sistema mejorado y el rendimiento original:

$$Speedup = \frac{Rendimiento_{Mejorado}}{Rendimiento_{Original}} = \frac{EX_{CPU\,Original}}{EX_{CPU\,Mejorado}}$$

La eficiencia mide el aprovechamiento de los recursos:

$$Eficiencia = \frac{Speedup_n}{n}$$

donde `n` es la cantidad de recursos (en este caso, núcleos).

### Speedup — basado en media geométrica general

Usando el i5-13600K como procesador de referencia (base = 67.41):

| Procesador mejorado | Rendimiento | Referencia | Speedup |
|---|---|---|---|
| Ryzen 9 5900X | 59.63 | i5-13600K (67.41) | 59.63 / 67.41 ≈ **0.88×** |
| Ryzen 9 7950X | 94.41 | i5-13600K (67.41) | 94.41 / 67.41 ≈ **1.40×** |
| Ryzen 9 7950X | 94.41 | Ryzen 9 5900X (59.63) | 94.41 / 59.63 ≈ **1.58×** |

El Ryzen 9 5900X tiene un speedup menor a 1 respecto al i5-13600K en rendimiento general, lo que significa que el i5 es más rápido en la mayoría de las cargas. El 7950X es el único que supera claramente al i5, con un 40% más de rendimiento general.

### Speedup — basado en compilación del kernel Linux

| Procesador mejorado | T_ref (s) | T_nuevo (s) | Speedup |
|---|---|---|---|
| Ryzen 9 5900X vs i5-13600K | 83 | ~76 | 83/76 ≈ **1.09×** |
| Ryzen 9 7950X vs i5-13600K | 83 | ~54 | 83/54 ≈ **1.54×** |
| Ryzen 9 7950X vs Ryzen 9 5900X | ~76 | ~54 | 76/54 ≈ **1.41×** |

En este benchmark puntual el 5900X sí supera al i5 (speedup > 1) por su mayor cantidad de hilos.

### Eficiencia por núcleo

| Procesador | Núcleos | Speedup vs i5 (general) | Eficiencia |
|---|---|---|---|
| i5-13600K (base) | 14 | 1.00× | 1.00 / 14 ≈ **0.071** |
| Ryzen 9 5900X | 12 | 0.88× | 0.88 / 12 ≈ **0.073** |
| Ryzen 9 7950X | 16 | 1.40× | 1.40 / 16 ≈ **0.088** |

### Eficiencia en costo (Performance/Dollar)

OpenBenchmarking.org también reporta la media geométrica por dólar invertido, usando los precios de referencia (i5-13600K: $320 — Ryzen 9 7950X: $569):

| Procesador | Precio ref. | Media geométrica/$ | Ranking costo |
|---|---|---|---|
| Intel Core i5-13600K | $320 | **0.211** | 1° |
| AMD Ryzen 9 7950X 16-Core | $569 | **0.166** | 2° |

El i5-13600K es más eficiente en términos de costo, ofreciendo mayor rendimiento por dólar que el 7950X. El 7950X tiene rendimiento absoluto superior pero su precio casi duplica al del i5, por lo que la elección depende de si la prioridad es el rendimiento máximo o la relación precio/rendimiento.

### Análisis

- El Ryzen 9 7950X lidera en rendimiento absoluto (media geométrica 94.41) y en eficiencia por núcleo (0.088), gracias a sus 16 núcleos Zen 4, soporte AVX-512 y alta frecuencia sostenida.
- El i5-13600K lidera en eficiencia por costo (0.211 puntos/$), siendo la mejor opción si el presupuesto es una restricción.
- El Ryzen 9 5900X, siendo de 2020 y arquitectura Zen 3, queda por debajo del i5-13600K en rendimiento general a pesar de tener más hilos, lo que demuestra que la generación de arquitectura importa tanto como el conteo de núcleos.

---

## Parte 4 — Profiling de código (gprof)

### ¿Qué es el profiling?

El profiling es el análisis experimental del tiempo de ejecución y uso de memoria de un programa. Las herramientas de profiling permiten validar la intuición del programador sobre la eficiencia de su código, yendo más allá del análisis de complejidad O grande. Dos técnicas principales:

- **Inyección de código (gprof):** se recompila el programa con flags especiales que insertan código en cada llamada/retorno de función para registrar tiempos.
- **Muestreo estadístico (perf):** el sistema operativo interrumpe el programa a intervalos fijos y registra qué función está ejecutándose.

### Pasos del tutorial gprof

#### Paso 1 — Compilar con profiling habilitado

```bash
gcc -Wall -pg test_gprof.c test_gprof_new.c -o test_gprof
```

La flag `-pg` indica al compilador que inserte código extra para que `gprof` pueda analizar el tiempo de ejecución de cada función.

#### Paso 2 — Ejecutar el programa

```bash
./test_gprof
```

Al ejecutarse, el binario genera automáticamente el archivo `gmon.out` en el directorio de trabajo.

#### Paso 3 — Ejecutar gprof y analizar

```bash
gprof test_gprof gmon.out > analysis.txt
```

El archivo `analysis.txt` contiene dos secciones principales:

**Flat profile** — resumen por función:

```
% cumulative self   calls  self   total  name
33.86   15.52   15.52    1  15.52  15.52  func2
33.82   31.02   15.50    1  15.50  15.50  new_func1
33.29   46.27   15.26    1  15.26  30.75  func1
0.07    46.30    0.03        0.03         main
```

- `%time`: porcentaje del tiempo total consumido por la función.
- `self seconds`: tiempo exclusivo de la función (sin sus hijos).
- `total seconds`: tiempo incluyendo las funciones que llama.
- `calls`: cuántas veces fue invocada.

**Call graph** — árbol de llamadas:

```
[1] 100.0  0.03  46.27    main [1]
              15.26  15.50  1/1  func1 [2]
              15.52   0.00  1/1  func2 [3]
[2]  66.4  15.26  15.50  1  func1 [2]
              15.50   0.00  1/1  new_func1 [4]
[3]  33.5  15.52   0.00  1  func2 [3]
[4]  33.5  15.50   0.00  1  new_func1 [4]
```

### Opciones útiles de gprof

| Flag | Efecto |
|---|---|
| `-a` | Suprime funciones estáticas (privadas) |
| `-b` | Elimina los textos descriptivos detallados |
| `-p` | Imprime solo el flat profile |
| `-pfunc1` | Muestra solo la función especificada en el flat profile |

### Conclusiones sobre el uso del tiempo

Del análisis del ejemplo del tutorial se observa que:

- Las tres funciones (`func1`, `func2`, `new_func1`) consumen tiempos similares (~33% cada una), ya que sus bucles `for` tienen límites muy parecidos.
- `func1` tiene un `total time` de 30.75 s porque engloba también el tiempo de `new_func1` (a quien llama).
- `main` consume menos del 0.1% del tiempo, siendo su rol simplemente orquestar las llamadas.
- El profiling confirma que los bucles `for` son los cuellos de botella, algo que en este caso era esperable pero que en código real puede sorprender.

### Alternativa: Linux perf

```bash
sudo perf record ./test_gprof
sudo perf report
```

`perf` usa muestreo estadístico en lugar de inyección de código, por lo que tiene menos impacto sobre el rendimiento del programa a analizar. Es útil cuando la recompilación no es posible o cuando se quiere evitar la distorsión que introduce `gprof`.

---

## Parte 5 — Práctico: efecto de la frecuencia de CPU en el tiempo de ejecución (ESP32 + Wokwi)

### Objetivo

Verificar experimentalmente la relación entre la frecuencia de la CPU y el tiempo de ejecución de un programa, contrastando el comportamiento teórico con los resultados obtenidos en simulación. Se utilizó el simulador online Wokwi ([wokwi.com/projects/new/esp32](https://wokwi.com/projects/new/esp32)) con una placa ESP32 DevKit C v4.

### Marco teórico

El tiempo de ejecución de un programa se define como:

$$T_{prog} = N°instrucciones \times CPI \times T_{CPU} = \frac{N°instrucciones \times CPI}{f_{CPU}}$$

Manteniendo constante el código (mismo número de instrucciones y mismo CPI), la relación esperada es de proporcionalidad inversa entre frecuencia y tiempo:

$$\frac{T_1}{T_2} = \frac{f_2}{f_1}$$

Por lo tanto, duplicar la frecuencia debería reducir el tiempo a la mitad.

### Código utilizado

Se utilizó un programa con dos bucles de carga computacional intensa. Se evitó el uso de bucles simples de suma, ya que el compilador GCC puede optimizarlos y eliminarlos, produciendo tiempos artificialmente bajos independientemente de la frecuencia.

El código final utiliza:
- Un generador de números pseudo-aleatorios LCG para el bucle de enteros, donde cada iteración depende de la anterior (impide la optimización por el compilador).
- La función `sqrtf()` para el bucle de floats, que fuerza trabajo real en la FPU.

```cpp
#include <Arduino.h>
#include <math.h>

void setup() {
    Serial.begin(115200);
    delay(1000);

    setCpuFrequencyMhz(240); // cambiar a 160 u 80 para cada corrida
    Serial.printf("CPU: %d MHz\n", getCpuFrequencyMhz());

    // --- Bucle con enteros (LCG pseudo-random) ---
    Serial.println("Iniciando suma de enteros...");
    unsigned long t0 = millis();

    volatile long suma = 0;
    volatile long x = 1;
    for (long i = 0; i < 10000000L; i++) {
        x = (x * 1664525L + 1013904223L) & 0x7FFFFFFF;
        suma += x;
    }

    unsigned long t1 = millis();
    Serial.printf("Resultado: %ld | Tiempo: %lu ms\n", suma, t1 - t0);

    // --- Bucle con floats (sqrtf) ---
    Serial.println("Iniciando suma de floats...");
    unsigned long t2 = millis();

    volatile float acum = 1.0f;
    for (long i = 1; i < 10000000L; i++) {
        acum += sqrtf((float)i) * 0.0001f;
    }

    unsigned long t3 = millis();
    Serial.printf("Resultado: %.4f | Tiempo: %lu ms\n", acum, t3 - t2);

    Serial.printf("Tiempo total: %lu ms\n", t3 - t0);
}

void loop() {}
```

### Entorno de simulación

| Parámetro | Valor |
|---|---|
| Plataforma | Wokwi ESP32 Simulator |
| Placa simulada | ESP32 DevKit C v4 |
| URL | https://wokwi.com/projects/new/esp32 |
| Compilador | GCC (Arduino Core para ESP32) |
| Iteraciones por bucle | 10.000.000 |

> **Nota sobre la simulación:** Wokwi simula el comportamiento funcional de la ESP32 pero no reproduce con exactitud el timing real del hardware (frecuencia de flash, wait states, caché de instrucciones). Los tiempos obtenidos son representativos del comportamiento relativo entre frecuencias, pero pueden diferir de los valores en hardware real.

### Resultados obtenidos

Se ejecutó el programa tres veces cambiando únicamente la llamada a `setCpuFrequencyMhz()`.

| Frecuencia (MHz) | T enteros (ms) | T floats (ms) | T total (ms) |
|---|---|---|---|
| 240 MHz | ~1.070 | ~8.950 | ~10.020 |
| 160 MHz | ~1.600 | ~13.430 | ~15.030 |
| 80 MHz | ~3.210 | ~26.860 | ~30.070 |

### Análisis de resultados

#### Relación entre frecuencia y tiempo (enteros)

Tomando 240 MHz como referencia:

| Comparación | Ratio de frecuencias | Ratio de tiempos medido | Ratio teórico esperado |
|---|---|---|---|
| 240 → 160 MHz | 240/160 = **1.50×** | 1600/1070 ≈ **1.50×** | 1.50× |
| 240 → 80 MHz | 240/80 = **3.00×** | 3210/1070 ≈ **3.00×** | 3.00× |

El bucle de enteros muestra una proporcionalidad inversa casi perfecta con la frecuencia, validando experimentalmente la fórmula $T_{prog} \propto 1/f_{CPU}$.

#### Diferencia entre enteros y floats

El bucle de floats tarda aproximadamente 8.4× más que el de enteros a igual frecuencia. Esto se debe a que `sqrtf()` involucra múltiples pasos de cálculo en la FPU (estimación inicial + iteraciones Newton-Raphson), mientras que las operaciones enteras del LCG son multiplicaciones y sumas simples. Cada iteración del bucle de floats tiene un CPI mucho mayor que la del bucle de enteros.

Esto ilustra que el tiempo de ejecución no depende solo de la frecuencia, sino también del CPI promedio de las instrucciones ejecutadas:

$$T_{prog} = N°instrucciones \times CPI \times \frac{1}{f_{CPU}}$$

Un programa con operaciones de mayor CPI (como `sqrtf`) tarda más incluso con el mismo número de iteraciones y la misma frecuencia.

#### Por qué el primer intento (suma simple) no mostraba diferencias

El bucle original `suma_int += i` es suficientemente simple para que GCC lo optimice durante la compilación, reemplazándolo por una fórmula aritmética directa. El resultado se calcula en pocas instrucciones independientemente de la frecuencia, haciendo que el cuello de botella pase a ser la inicialización del Serial y los `delay()`, que son fijos. Al introducir dependencias de datos entre iteraciones se fuerza la ejecución real de cada iteración.

### Conclusiones

1. La relación $T \propto 1/f_{CPU}$ se verifica experimentalmente cuando el código ejecuta trabajo real que el compilador no puede optimizar.
2. El **CPI no es constante** entre tipos de operaciones: las operaciones de punto flotante complejas (`sqrtf`) tienen un CPI significativamente mayor que las operaciones enteras simples.
3. La herramienta de simulación **Wokwi** permite reproducir el efecto de cambio de frecuencia de forma conveniente sin necesidad de hardware físico, aunque sus tiempos absolutos pueden diferir del hardware real por no modelar exactamente la latencia de la flash ni el comportamiento del caché.
4. Para medir rendimiento de forma confiable es necesario asegurarse de que el compilador **no elimine el código bajo prueba**, usando `volatile`, dependencias entre iteraciones, o funciones con efectos secundarios verificables.

---

## Referencias

- OpenBenchmarking.org — `pts/build-linux-kernel`
- Phoronix Test Suite — resultados para i5-13600K, Ryzen 9 5900X y 7950X
- Tutorial gprof: Himanshu Arora (2012), adaptado por Javier Jorge 
- Wokwi ESP32 Simulator — https://wokwi.com/projects/new/esp32
- Material de cátedra: *El rendimiento de las computadoras* y *Time Profiling (GPROF & Perf)*