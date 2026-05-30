# gpio_driver — Etapa 2 del TP (GPIO mapeado en memoria)

> 📄 **El informe completo está en el [`README.md` de la raíz](../README.md).**
> Allí se documenta el recorrido completo del trabajo: fundamentos del CDD, la etapa
> ADC/IIO (`sdec_cdd`), esta etapa GPIO (`gpio_cdd`), el experimento de carga de CPU
> y las conclusiones. Este directorio contiene el código de la **segunda etapa** y las
> notas de build específicas de la cross-compilación.

## Contenido

| Archivo | Qué es |
|---|---|
| `gpio_cdd.c` | CDD que lee GPIO del AM335x con `ioremap` + `readl` (`/dev/SdC_gpio`) |
| `drv1.c` … `drv4.c` | Progresión paso a paso del esqueleto de un CDD |
| `clipboard.c` | Módulo `/proc` con `proc_ops` |
| `test_gpio.sh` | Script de prueba del driver |
| `Makefile` | Build con cross-compilación (`ARCH=arm`, `CROSS_COMPILE=arm-linux-gnueabihf-`) |
| `app/signal_server.py` | Servidor web (Chart.js sobre HTTP) — corre en la BBB |

## Build rápido (cross-compilación)

Para el detalle conceptual (target triple, `HOSTCC` vs `CC`, el problema de `modpost`)
ver la sección **«Etapa 2 — CDD sobre GPIO»** del informe raíz.

```bash
# 1. Toolchain (una vez):
sudo apt install gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf

# 2. Versión del kernel de la BBB (arma la ruta a los headers):
make set-version BBB=debian@10.42.0.228     # → escribe .bbb-kernel-version

# 3. Compilar y verificar que el .ko es ARM:
make
file gpio_cdd.ko        # → ELF 32-bit LSB relocatable, ARM

# 4. Desplegar a la BBB:
make deploy BBB=debian@10.42.0.228
```

## Carga en la BBB

```bash
sudo config-pin P9_12 gpio && sudo config-pin P9_14 gpio   # pinmux
sudo insmod ~/gpio_driver/gpio_cdd.ko
sudo chmod a+rw /dev/SdC_gpio
python3 ~/gpio_driver/signal_server.py                     # http://10.42.0.228:8080
```

Parámetros del módulo: `gpio_bank` (default 1), `gpio_bits` (default `28,18`), `sample_ms` (default 100).
