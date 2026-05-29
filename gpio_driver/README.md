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
