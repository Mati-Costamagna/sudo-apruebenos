# Device Drivers — GPIO en BeagleBone Black

**Asignatura:** Sistemas de Computación

**Profesores:**
- Jorge, Javier Alejandro
- Solinas, Miguel Angel

**Estudiantes:**
- Costamagna, Matias
- Davila Tomassi, Carlos Valentino
- Sabena, Maria Pilar

**Fecha:** Junio 2026

---

## Introducción

Este informe documenta la construcción de un CDD sobre una **BeagleBone Black**, siguiendo la progresión de la consigna (drv1 → drv4 → clipboard → hardware GPIO). La señal de entrada es un generador de pulsos externo conectado a los pines GPIO del header P9.

El flujo de trabajo sigue el enfoque de **cross-compilación**: todo el código se escribe en la PC anfitriona, se compila allí apuntando a la arquitectura ARM del AM335x, y los binarios se transfieren a la BBB por SSH.

## Werner Almesberger

Werner Almesberger es un ingeniero de software suizo conocido por sus contribuciones al kernel de Linux, en particular por ser el autor original del soporte para el sistema de archivos **FAT/VFAT** en Linux (módulo `fs/fat`). Su nombre aparece en los comentarios de ese subsistema. También trabajó en el proyecto LILO y fue uno de los primeros contribuidores del kernel en los años 90.

```bash
ls /lib/modules/$(uname -r)/kernel/fs/fat/
modinfo fat | grep author
```

---

## Entorno de desarrollo y cross-compilación

### ¿Qué es cross-compilación?

Compilar en una arquitectura (PC x86_64) para que el binario resultante corra en otra (BBB ARM Cortex-A8). El compilador nativo de la PC genera código x86 que la BBB no puede ejecutar; el **cross-compiler** genera código ARM.

```
PC (x86_64)                          BBB (ARM Cortex-A8)
┌─────────────────────┐              ┌──────────────────────┐
│ Editor (VSCodium)   │              │                      │
│ gpio_cdd.c          │  scp .ko     │  sudo insmod         │
│      ↓              │ ──────────▶  │  gpio_cdd.ko         │
│ arm-linux-gnueabihf-│              │                      │
│ -gcc  →  gpio_cdd.ko│              │  /dev/SdC_gpio       │
└─────────────────────┘              └──────────────────────┘
```

### 1. Instalar el toolchain en la PC

```bash
sudo apt install gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf
```

Verificar:
```bash
arm-linux-gnueabihf-gcc --version
# arm-linux-gnueabihf-gcc (Ubuntu ...) 11.x.x
```

### 2. Obtener los headers del kernel de la BBB

Los headers deben coincidir **exactamente** con la versión del kernel que corre en la BBB. Hacerlo una vez y guardarlos en la PC.

```bash
# En la BBB — instalar headers y empaquetar:
sudo apt install linux-headers-$(uname -r)
tar czf bbb-headers.tar.gz /usr/src/linux-headers-$(uname -r)

# En la PC — copiar y extraer:
scp debian@10.42.0.228:~/bbb-headers.tar.gz .
mkdir -p ~/bbb-kernel
tar xzf bbb-headers.tar.gz -C ~/bbb-kernel/
```

### 3. Guardar la versión del kernel

El Makefile usa el archivo `.bbb-kernel-version` para construir la ruta a los headers automáticamente:

```bash
# En el directorio gpio_driver/ de la PC:
make set-version BBB=debian@10.42.0.228
# Guarda la versión en .bbb-kernel-version
```

O manualmente:
```bash
ssh debian@10.42.0.228 "uname -r" > .bbb-kernel-version
cat .bbb-kernel-version
# 5.10.168-ti-r61
```

### 4. Compilar desde la PC

```bash
cd gpio_driver/
make
```

El Makefile invoca al sistema de build del kernel con `ARCH=arm` y `CROSS_COMPILE=arm-linux-gnueabihf-`. El resultado es `gpio_cdd.ko` compilado para ARM.

Verificar que el binario es para ARM:
```bash
file gpio_cdd.ko
# gpio_cdd.ko: ELF 32-bit LSB relocatable, ARM, EABI5 version 1 (SYSV)...
```

### 5. Transferir a la BBB

```bash
# Atajo del Makefile:
make deploy BBB=debian@10.42.0.228

# O manualmente:
ssh debian@10.42.0.228 "mkdir -p ~/gpio_driver"
scp gpio_cdd.ko debian@10.42.0.228:~/gpio_driver/
scp app/signal_server.py debian@10.42.0.228:~/gpio_driver/
```

---

## Cross-compilación en profundidad

### El target triple

El toolchain se identifica mediante un **target triple** con la forma `arquitectura-sistema-ABI`:

```
arm-linux-gnueabihf
 │     │      │
 │     │      └─ gnueabihf: GNU, EABI hard-float
 │     │           → usa registros de punto flotante del hardware (VFP)
 │     │           → en lugar de emular FP por software
 │     └─ linux: kernel Linux (syscall interface)
 └─ arm: arquitectura destino (ARMv7-A, Cortex-A8 en el AM335x)
```

El prefijo `arm-linux-gnueabihf-` se antepone a cada herramienta del toolchain:

| Herramienta | Función |
|---|---|
| `arm-linux-gnueabihf-gcc` | Compilador C para ARM |
| `arm-linux-gnueabihf-ld` | Linker para ARM |
| `arm-linux-gnueabihf-objdump` | Inspección de binarios ARM |
| `arm-linux-gnueabihf-strip` | Eliminar símbolos de debug |

### ARCH y CROSS_COMPILE en el sistema de build del kernel

El sistema de build del kernel (Kbuild) usa dos variables para el cross-compiling:

```makefile
ARCH          := arm
CROSS_COMPILE := arm-linux-gnueabihf-
```

- `ARCH` selecciona el árbol de arquitectura dentro del kernel (`arch/arm/`), determina las instrucciones generadas y los headers específicos de la arquitectura.
- `CROSS_COMPILE` es el prefijo que Kbuild antepone a `gcc`, `ld`, `objcopy`, etc. para usar el toolchain correcto en lugar del compilador nativo del host.

Sin estas variables, `make` usaría el `gcc` nativo de la PC y generaría código x86_64 que la BBB no puede ejecutar.

### HOSTCC vs CC: herramientas host y herramientas target

El sistema de build del kernel distingue dos tipos de herramientas:

| Variable | Compilador | Genera código para | Uso |
|---|---|---|---|
| `CC` | `arm-linux-gnueabihf-gcc` | ARM (BBB) | Módulos `.ko`, código del kernel |
| `HOSTCC` | `gcc` (nativo x86_64) | x86_64 (PC) | Herramientas del build que deben correr en la PC |

Las herramientas declaradas como `hostprogs` en los Makefiles del kernel (como `modpost`, `mk_elfconfig`, `bin2c`) se compilan con `HOSTCC` porque necesitan **ejecutarse en la PC** durante el proceso de build, no en la BBB.

### El problema de modpost

`modpost` es la herramienta que procesa los símbolos de los módulos kernel y genera los archivos `.mod.c`. Es un `hostprog`: debe correr en la PC (x86_64) durante la compilación.

Los headers del kernel extraídos de la BBB incluían un binario `modpost` compilado para ARM (ya que provienen de la placa). Al intentar ejecutarlo en la PC durante el build, el kernel fallaba silenciosamente o con error de formato.

```bash
file scripts/mod/modpost
# ELF 32-bit LSB pie executable, ARM  ← no puede correr en x86_64
```

La solución fue recompilarlo para el host **sin** usar los headers del kernel como include path:

```bash
cd $KDIR/scripts/mod
gcc -o modpost modpost.c file2alias.c sumversion.c
# ELF 64-bit LSB pie executable, x86-64  ← correcto
```

El error al intentar compilarlo con `-I$KDIR/include` proviene de que los headers del kernel ARM definen tipos como `uint64_t`, `loff_t` y `dev_t` con tamaños ARM, que colisionan con las definiciones de glibc para x86_64. `modpost` es una herramienta del host y solo necesita headers del sistema.

### Verificación del binario generado

Después de compilar, siempre verificar que el `.ko` es para la arquitectura correcta:

```bash
file gpio_cdd.ko
# gpio_cdd.ko: ELF 32-bit LSB relocatable, ARM, EABI5 version 1 (SYSV)
#              ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#              Confirma: ARM, 32-bit, relocatable (módulo, no ejecutable)
```

Un módulo es un objeto **relocatable** (no un ejecutable): no tiene dirección base fija. El kernel lo carga en memoria y resuelve las referencias a símbolos del kernel en tiempo de carga (`insmod`).

---

## Pasos 1–5: drv1 a clipboard

La progresión drv1 → drv4 → clipboard es idéntica a cualquier plataforma Linux. Estos módulos no tienen código específico de hardware, se compilan con el mismo Makefile y se transfieren y cargan igual en la BBB.

| Paso | Archivo | Lo que agrega |
|---|---|---|
| 1 | `drv1.c` | `module_init` / `module_exit` |
| 2 | `drv2.c` | `alloc_chrdev_region` — major/minor, nodos manuales con `mknod` |
| 3 | `drv3.c` | `cdev` + `file_operations` + `class`/`device_create` — auto `/dev` |
| 4 | `drv4.c` | `copy_to_user` / `copy_from_user` — transferencia real de datos |
| 5 | `clipboard.c` | `/proc` con `proc_ops` |

---

## Paso 6 — gpio_cdd: CDD con GPIO mapeado en memoria

### Hardware del AM335x

El SoC AM335x tiene cuatro bancos GPIO con registros en direcciones físicas fijas:

| Banco | Base física  | Pines Linux | Ejemplos en P9             |
|-------|-------------|-------------|----------------------------|
| GPIO0 | `0x44E07000`| 0 – 31      | P9_11 (GPIO0[30])          |
| GPIO1 | `0x4804C000`| 32 – 63     | P9_12 (GPIO1[28]), P9_14 (GPIO1[18]) |
| GPIO2 | `0x481AC000`| 64 – 95     | P8_7 (GPIO2[2])            |
| GPIO3 | `0x481AE000`| 96 – 127    | P9_25 (GPIO3[21])          |

Número Linux del GPIO: `bank × 32 + bit`. Ejemplo: GPIO1[28] → pin 60.

### Registros utilizados

| Registro     | Offset | Descripción |
|---|---|---|
| `GPIO_OE`    | `0x134` | Output Enable. **bit=1 = entrada**, bit=0 = salida |
| `GPIO_DATAIN`| `0x138` | Nivel lógico actual del pin |

`GPIO_OE` tiene lógica inversa: un `1` **deshabilita** el driver de salida, configurando el pin como entrada.

### Hardware de prueba

| Componente | Cantidad | Función |
|---|---|---|
| BeagleBone Black | 1 | Placa principal |
| Generador de pulsos | 2 | Señal digital en P9_12 (canal 0) y P9_14 (canal 1) |
| Resistor 10 kΩ | 2 | Pull-down |

```
Generador ──[ 10kΩ ]──── P9_12 (GPIO1[28])  ← canal 0
                     │
                    GND (P9_1)

Generador ──[ 10kΩ ]──── P9_14 (GPIO1[18])  ← canal 1
                     │
                    GND
```

**Tensión máxima en los pines GPIO del AM335x: 3.3 V.**

### Prerrequisito: pinmux

Antes de cargar el módulo, configurar los pines en modo GPIO:

```bash
# En la BBB:
sudo config-pin P9_12 gpio
sudo config-pin P9_14 gpio

config-pin -q P9_12   # → P9_12 Mode: gpio
config-pin -q P9_14   # → P9_14 Mode: gpio
```

### Cargar el módulo

```bash
# En la BBB:
sudo insmod ~/gpio_driver/gpio_cdd.ko
dmesg | tail -3
# SdC_gpio: inicializado — major=237, GPIO1[28,18], T=100ms

sudo chmod a+rw /dev/SdC_gpio

# Verificar manualmente:
cat /dev/SdC_gpio          # 0 o 1
echo 1 > /dev/SdC_gpio    # cambiar a canal 1
cat /dev/SdC_gpio
```

---

## Aplicación web de visualización

La aplicación corre en la BBB, lee `/dev/SdC_gpio` y sirve los datos por HTTP. La visualización se abre en el navegador de la PC.

```
BBB                                  PC (navegador)
┌─────────────────────────┐          ┌──────────────────────────┐
│  signal_server.py       │          │                          │
│  └─ lee /dev/SdC_gpio   │  HTTP    │  http://10.42.0.228:8080 │
│  └─ buffer circular     │ ───────▶ │  Chart.js                │
│  └─ HTTP en :8080       │  /data   │  gráfico en tiempo real  │
└─────────────────────────┘          └──────────────────────────┘
```

### Ejecutar en la BBB

```bash
python3 ~/gpio_driver/signal_server.py
# Device : /dev/SdC_gpio
# Puerto : http://0.0.0.0:8080
# Muestreo: 100 ms (10 Hz)
# Abrir en la PC: http://10.42.0.228:8080
```

Opciones:
```bash
# Muestreo más rápido (10 ms = 100 Hz):
python3 signal_server.py --interval 10

# Puerto distinto:
python3 signal_server.py --port 9000

# Device alternativo:
python3 signal_server.py --device /dev/SdC_gpio --interval 50
```

### ¿Qué hace la app?

- Un hilo de fondo muestrea ambos canales del driver cada `interval` ms y llena dos buffers circulares de 300 puntos.
- El servidor HTTP sirve en `/` la página HTML con Chart.js embebido.
- El endpoint `/data` devuelve JSON con tiempos y valores de ambos canales.
- El navegador hace polling a `/data` cada 200 ms y actualiza el gráfico sin recargar la página.
- El gráfico usa modo `stepped: true` para representar correctamente la señal digital (escalones en lugar de líneas suaves).

### Parámetros del módulo gpio_cdd

| Parámetro | Default | Descripción |
|---|---|---|
| `gpio_bank` | `1` | Banco GPIO (0–3) |
| `gpio_bits` | `28,18` | Bits dentro del banco (canal 0 y canal 1) |
| `sample_ms` | `100` | Período de muestreo del timer en ms |

```bash
# Usar GPIO2, pines P8_7 y P8_8 (GPIO2[2] y GPIO2[3]):
sudo insmod gpio_cdd.ko gpio_bank=2 gpio_bits=2,3

# Muestreo más frecuente:
sudo insmod gpio_cdd.ko sample_ms=10
```

---

## Efecto de la carga del CPU en el muestreo

### Prueba realizada

Con el servidor `signal_server.py` corriendo en segundo plano y el módulo `gpio_cdd.ko` activo, se ejecutó simultáneamente un loop intensivo de punto flotante en la misma BBB:

```bash
echo -e "import math,time\ni=0\nwhile True:\n    i+=1;t0=time.perf_counter();r=sum(math.sin(j)*math.cos(j)+math.sqrt(j)*math.log(j+1)+math.atan(j)*math.exp(j%10)+math.sinh(j%20)*math.cosh(j%20)+math.pow(j%100,2.7183) for j in range(1,200001));print(f'ciclo {i} | {(time.perf_counter()-t0)*1000:.1f} ms')" | python3
```

El loop realiza por ciclo 200 000 operaciones trigonométricas, logarítmicas, hiperbólicas y potencias con exponente irracional — operaciones costosas para la FPU del Cortex-A8.

### Resultado observado

Con el CPU bajo alta carga, la señal visualizada en el gráfico web presentó distorsión: los flancos de la señal digital se veían irregulares y el período aparente variaba, a pesar de que la señal del generador era constante.

### Teoría implicada

El muestreo en `gpio_cdd.ko` se realiza mediante un **timer del kernel** (`mod_timer`), que agenda callbacks con resolución de jiffies (1 ms por defecto en el AM335x). Este timer **no es de tiempo real**: el kernel puede demorar su ejecución si el CPU está ocupado atendiendo otra tarea de mayor prioridad o un proceso en espacio de usuario que monopoliza el scheduler.

Cuando el loop de punto flotante carga el CPU:

1. **Jitter en el timer**: el callback `timer_callback` no se ejecuta exactamente cada `sample_ms` sino con retardo variable, porque el scheduler CFS (Completely Fair Scheduler) reparte el CPU entre el proceso de carga y las tareas del kernel.

2. **Latencia en el servidor web**: el hilo de muestreo de `signal_server.py` también compite por CPU, produciendo intervalos de muestreo irregulares en espacio de usuario.

3. **Efecto combinado**: la irregularidad en ambos lados (kernel y userspace) se suma, resultando en una representación temporal distorsionada de la señal en el gráfico.

### Conclusión

Para mediciones precisas de señales digitales en un sistema Linux no-RT, la carga del CPU afecta directamente la fidelidad temporal del muestreo por polling y timers. Si se requiere precisión en los flancos, la solución correcta es usar **interrupciones GPIO** (`request_irq` con `IRQF_TRIGGER_RISING/FALLING`) en lugar de timers periódicos, ya que las interrupciones tienen prioridad sobre el scheduler y no dependen de la carga del sistema.

---

## Resumen del flujo completo

```
PC (desarrollo)                      BBB (ejecución)
───────────────                      ───────────────
1. Escribir gpio_cdd.c
2. make                ──── scp ───▶ 3. sudo insmod gpio_cdd.ko
                                     4. config-pin P9_12/P9_14 gpio
5. Escribir             ── scp ───▶  6. python3 signal_server.py
   signal_server.py
                                            │ HTTP :8080
PC (navegador)  ◀───────────────────────────┘
7. http://10.42.0.228:8080
   → gráfico en tiempo real de la señal GPIO
```
