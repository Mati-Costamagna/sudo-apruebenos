# TP1 — El Rendimiento de las Computadoras

## Parte 1 — Lista de benchmarks por tarea diaria

La mejor forma de evaluar el rendimiento de un sistema es medir el tiempo que tarda en ejecutar los programas que el usuario realmente utiliza. A continuación se presenta una tabla de doble entrada con tareas cotidianas y el benchmark que mejor las representa.

| Tarea diaria | Benchmark representativo | Tipo | Fuente |
|---|---|---|---|
| Compilar proyectos de software | Timed Linux Kernel Compilation (`pts/build-linux-kernel`) | Kernel | openbenchmarking.org |
| Cálculo numérico / simulaciones | LINPACK / HPL (mide GFLOPS) | Sintético | top500.org / pts/linpack |
| Trabajo con bases de datos | pgbench / sysbench (transacciones por segundo) | Kernel | pts/pgbench |
| Uso general de escritorio | PCMark 10 / Geekbench 6 | Sintético | futuremark.com |
| Compresión y archivado | 7-Zip benchmark (`pts/compress-7zip`) | Kernel | openbenchmarking.org |

**Tipos de benchmark:**
- **Sintético:** programa pequeño y enfocado en un tipo de carga específico.
- **Kernel:** extracto representativo de una carga de trabajo real.

---

## Parte 2 — Rendimiento en compilación del kernel Linux

**Benchmark utilizado:** `pts/build-linux-kernel`  
**Carga de trabajo:** compilación de Linux con configuración `defconfig`, arquitectura x86_64.  
**Métrica:** segundos totales de compilación — menor es mejor.  
**Fuente:** OpenBenchmarking.org (datos verificados con capturas del sitio)

### Especificaciones de los procesadores (fuente: OpenBenchmarking.org)

| Procesador | Núcleos | Hilos | CPU Clock | Core Family | Año | Percentil general |
|---|---|---|---|---|---|---|
| AMD Ryzen 9 5900X 12-Core | 12 | 24 | 3.7 GHz | Zen 3 | 2020 | 52° |
| Intel Core i5-13600K | 14 | 20 | 5.1 GHz | Raptor Lake | 2022 | 47° |
| AMD Ryzen 9 7950X 16-Core | 16 | 32 | 4.5 GHz | Zen 4 | 2022 | 71° |

### Rendimiento general — Media geométrica de todos los benchmarks

La media geométrica resume el rendimiento relativo a través de los 348 benchmarks en común registrados en OpenBenchmarking.org. Mayor es mejor.

| Procesador | Media geométrica | Ranking |
|---|---|---|
| AMD Ryzen 9 7950X 16-Core | **95.03** | 1° |
| Intel Core i5-13600K | **67.80** | 2° |
| AMD Ryzen 9 5900X 12-Core | **59.97** | 3° |

> El i5-13600K supera al Ryzen 9 5900X en rendimiento general a pesar de tener menos hilos (20 vs 24), gracias a su arquitectura más nueva (Raptor Lake 2022 vs Zen 3 2020) y mayor frecuencia de clock (5.1 GHz vs 3.7 GHz).

### Rendimiento en compilación del kernel Linux

| Procesador | Tiempo aprox. (s) | Percentil compilación |
|---|---|---|
| Intel Core i5-13600K | ~72 s | 52° |
| AMD Ryzen 9 5900X 12-Core | ~76 s | 47° |
| AMD Ryzen 9 7950X 16-Core | ~50 s | 71° |

> En este benchmark específico el i5-13600K supera levemente al 5900X (~72 s vs ~76 s) a pesar de tener menos hilos (20 vs 24). La ventaja arquitectónica de Raptor Lake y su mayor frecuencia de clock compensan la diferencia de hilos, incluso en una carga paralela como `make -j`. Esto ilustra que el rendimiento depende no solo de la cantidad de núcleos/hilos, sino también de la microarquitectura y la frecuencia.

---

## Parte 3 — Speedup y eficiencia del Ryzen 9 7950X

### Definiciones

El speedup es la razón entre el rendimiento del sistema mejorado y el rendimiento original:

$$Speedup = \frac{Rendimiento_{Mejorado}}{Rendimiento_{Original}} = \frac{EX_{CPU\,Original}}{EX_{CPU\,Mejorado}}$$

La eficiencia mide el aprovechamiento de los recursos:

$$Eficiencia = \frac{Speedup_n}{n}$$

donde `n` es la cantidad de recursos (en este caso, núcleos).

### Speedup — basado en media geométrica general

Usando el i5-13600K como procesador de referencia (base = 67.80):

| Procesador mejorado | Rendimiento | Referencia | Speedup |
|---|---|---|---|
| Ryzen 9 5900X | 59.97 | i5-13600K (67.80) | 59.97 / 67.80 ≈ **0.88×** |
| Ryzen 9 7950X | 95.03| i5-13600K (67.80) | 95.03 / 67.80 ≈ **1.40×** |
| Ryzen 9 7950X | 95.03 | Ryzen 9 5900X (59.97) | 95.03 / 59.97 ≈ **1.58×** |

El Ryzen 9 5900X tiene un speedup menor a 1 respecto al i5-13600K en rendimiento general, lo que significa que el i5 es más rápido en la mayoría de las cargas. El 7950X es el único que supera claramente al i5, con un 40% más de rendimiento general.

### Speedup — basado en compilación del kernel Linux

| Procesador mejorado | T_ref (s) | T_nuevo (s) | Speedup |
|---|---|---|---|
| Ryzen 9 5900X vs i5-13600K | 72 | ~76 | 72/76 ≈ **0.94x** |
| Ryzen 9 7950X vs i5-13600K | 72 | ~50 | 72/50 ≈ **1.44x** |
| Ryzen 9 7950X vs Ryzen 9 5900X | ~76 | ~50 | 76/50 ≈ **1.52x** |

En compilación, el 5900X también queda por debajo del i5 (speedup 0.94×, menor que 1), consistente con lo observado en la Parte 2. El 7950X es el único que supera claramente al i5 en ambas métricas (general y compilación).

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
| Intel Core i5-13600K | $320 | **0.212** | 1° |
| AMD Ryzen 9 7950X 16-Core | $569 | **0.167** | 2° |

El i5-13600K es más eficiente en términos de costo, ofreciendo mayor rendimiento por dólar que el 7950X. El 7950X tiene rendimiento absoluto superior pero su precio casi duplica al del i5, por lo que la elección depende de si la prioridad es el rendimiento máximo o la relación precio/rendimiento.

### Análisis

- El Ryzen 9 7950X lidera en rendimiento absoluto (media geométrica 95.03) y en eficiencia por núcleo (0.088), gracias a sus 16 núcleos Zen 4, soporte AVX-512 y alta frecuencia sostenida.
- El i5-13600K lidera en eficiencia por costo (0.212 puntos/$), siendo la mejor opción si el presupuesto es una restricción.
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

```cpp
void setup() {
    Serial.begin(9600);
    delay(1000);

    setCpuFrequencyMhz(8);   // o 16, o 4
    Serial.printf("CPU: %d MHz\n", getCpuFrequencyMhz());
    // --- Bucle con enteros ---
    Serial.println("Iniciando suma de enteros...");
    unsigned long t0 = millis();

    volatile long suma_int = 0;
    for (long i = 0; i < 250000L; i++) {
        suma_int += i;
    }

    unsigned long t1 = millis();
    Serial.print("Suma enteros: ");
    Serial.println(suma_int);
    Serial.print("Tiempo: ");
    Serial.print(t1 - t0);
    Serial.println(" ms");

    // --- Bucle con floats ---
    Serial.println("Iniciando suma de floats...");
    unsigned long t2 = millis();

    volatile float suma_float = 0.0f;
    for (long i = 0; i < 10000000L; i++) {
        suma_float += (float)i * 0.1f;
    }

    unsigned long t3 = millis();
    Serial.print("Suma floats: ");
    Serial.println(suma_float);
    Serial.print("Tiempo: ");
    Serial.print(t3 - t2);
    Serial.println(" ms");

    // --- Tiempo total ---
    Serial.print("Tiempo total: ");
    Serial.print(t3 - t0);
    Serial.println(" ms");
}

void loop() { }
```

### Entorno de simulación

| Parámetro | Valor |
|---|---|
| Plataforma | Wokwi ESP32 Simulator |
| Placa simulada | ESP32 DevKit C v4 |
| URL | https://wokwi.com/projects/new/esp32 |
| Compilador | GCC (Arduino Core para ESP32) |
| Iteraciones por bucle | 10.000.000 |

> **Nota sobre la simulación:** Wokwi capea la frecuencia de CPU simulada a ~8 MHz por defecto (modo `"auto"`). Frecuencias configuradas por encima de este límite se ejecutan a la velocidad del cap, no a la frecuencia solicitada. Esto afecta la proporcionalidad entre frecuencia y tiempo de ejecución en los resultados.

### Resultados obtenidos

Se ejecutó el programa tres veces cambiando únicamente la llamada a `setCpuFrequencyMhz()`.

| Frecuencia (MHz) | T enteros (ms) | T floats (ms) | T total (ms) |
|---|---|---|---|
| 16 MHz | ~164 | ~9.862 | ~10.027 |
| 8 MHz | ~198 | ~11.989 | ~12.189 |
| 4 MHz | ~232 | ~15.000 | ~15.237 |

### Análisis de resultados

#### Relación entre frecuencia y tiempo

Tomando 16 MHz como referencia y usando los tiempos totales:

| Comparación | Ratio de frecuencias | Ratio de tiempos medido | Ratio teórico esperado |
|---|---|---|---|
| 16 → 8 MHz | 16/8 = **2.00×** | 12189/10027 ≈ **1.22×** | 2.00× |
| 16 → 4 MHz | 16/4 = **4.00×** | 15237/10027 ≈ **1.52×** | 4.00× |

Los resultados muestran que los tiempos medidos no escalan proporcionalmente con la reducción de frecuencia. Al reducir la frecuencia a la mitad (16→8 MHz), el tiempo aumenta solo un 22% en vez del 100% esperado. Al reducir a un cuarto (16→4 MHz), el tiempo aumenta un 52% en vez del 300% esperado. Esto no valida la fórmula $T_{prog} \propto 1/f_{CPU}$, y la causa es una limitación del simulador (ver sección siguiente).

El tiempo de ejecución no depende solo de la frecuencia, sino también del número de instrucciones y del CPI promedio:

$$T_{prog} = N°instrucciones \times CPI \times \frac{1}{f_{CPU}}$$

Un programa con más iteraciones y operaciones de mayor CPI (como multiplicación en punto flotante) tarda más incluso a la misma frecuencia.

#### Por qué la simulación no refleja la relación teórica

Wokwi sí modela distintas velocidades de CPU, pero aplica un cap de frecuencia que distorsiona los resultados. Según la documentación oficial del simulador:

> *CPU frequency limit — In order to achieve a higher simulation speed, Wokwi automatically limits the maximum simulated CPU frequency. [...] The CPU frequency limit does not affect the timing of the peripherals, only the speed instructions are executed. [...] The default value is "auto", which means that Wokwi will automatically cap the CPU frequency to about 8 MHz.*

Esto explica el comportamiento observado:

1. **Cap de ~8 MHz por defecto.** Cuando se configura `setCpuFrequencyMhz(16)`, el simulador capea la ejecución real a ~8 MHz. Esto significa que las mediciones a 16 MHz y 8 MHz reflejan velocidades de ejecución similares (ambas cerca del cap), lo que explica por qué la diferencia entre ellas (22%) es menor que la teórica (100%).
2. **Por debajo del cap sí hay efecto.** A 4 MHz (por debajo del cap de 8 MHz), el simulador ejecuta las instrucciones a la velocidad configurada, y se observa un aumento mayor del tiempo (52% vs 16 MHz). Sin embargo, como la referencia de 16 MHz ya está capeada a ~8 MHz, la comparación no refleja un ratio real de 4:1 sino más bien de ~2:1 (8 MHz capeado → 4 MHz real).
3. **Los periféricos no se ven afectados.** La documentación aclara que el cap solo afecta la velocidad de ejecución de instrucciones, no el timing de periféricos como `millis()`, lo que permite que las mediciones de tiempo sigan siendo válidas en términos relativos.

Para verificar experimentalmente la relación $T \propto 1/f_{CPU}$ con proporcionalidad exacta se necesitaría hardware real (una ESP32 física) o configurar el atributo `"cpuFrequency": "max"` en Wokwi (lo cual intentamos realizar pero comprobamos lo dicho en la documentación de que hace excesivamente lenta la simulación).

### Conclusiones

1. La relación teórica $T \propto 1/f_{CPU}$ no pudo verificarse con proporcionalidad exacta en esta simulación. Al reducir la frecuencia de 16 a 4 MHz (ratio teórico 4×), el tiempo solo aumentó un 52% (ratio medido 1.52×).
2. La causa principal es el cap de frecuencia de Wokwi (~8 MHz por defecto): frecuencias configuradas por encima del cap se ejecutan a la misma velocidad, distorsionando las mediciones. Por debajo del cap sí se observa un efecto parcial del cambio de frecuencia.
3. El CPI no es constante entre tipos de operaciones: las operaciones de punto flotante (multiplicación float) tienen un CPI mayor que las operaciones enteras simples (suma), lo cual se refleja en la diferencia de tiempos entre ambos bucles.
4. Para medir rendimiento de forma confiable es necesario: (a) usar hardware real o un simulador cycle-accurate, y (b) asegurarse de que el compilador no elimine el código bajo prueba, usando `volatile` o dependencias entre iteraciones.
5. La simulación sí es útil para verificar el comportamiento funcional del código (que la API `setCpuFrequencyMhz` funciona, que los cálculos dan resultados correctos), pero no para medir efectos de rendimiento ligados a la frecuencia.

---

## Referencias

- OpenBenchmarking.org — `pts/build-linux-kernel`
- Phoronix Test Suite — resultados para i5-13600K, Ryzen 9 5900X y 7950X
- Tutorial gprof: Himanshu Arora (2012), adaptado por Javier Jorge 
- Wokwi ESP32 Simulator — https://wokwi.com/projects/new/esp32
- Material de cátedra: *El rendimiento de las computadoras* y *Time Profiling (GPROF & Perf)*
