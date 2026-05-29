#!/bin/bash
# Test para gpio_cdd.ko — ejecutar en la BeagleBone Black
# Uso: sudo bash test_gpio.sh [bank] [bit0] [bit1] [sample_ms]
# Defaults: bank=1, bit0=28, bit1=18, sample_ms=100
#
# Ejemplo con P9_12 y P9_14 (GPIO1[28] y GPIO1[18]):
#   sudo bash test_gpio.sh
# Ejemplo con GPIO2[2] y GPIO2[3] (P8_7 y P8_8), muestreo a 10ms:
#   sudo bash test_gpio.sh 2 2 3 10
set -e

BANK=${1:-1}
BIT0=${2:-28}
BIT1=${3:-18}
SAMPLE_MS=${4:-100}

DEVICE=/dev/SdC_gpio
MODULE=gpio_cdd

echo "=== Configuración ==="
echo "  Banco: GPIO${BANK}"
echo "  Canal 0: GPIO${BANK}[${BIT0}]"
echo "  Canal 1: GPIO${BANK}[${BIT1}]"
echo "  Período de muestreo: ${SAMPLE_MS} ms"

echo ""
echo "=== Cargando módulo ==="
sudo insmod ./${MODULE}.ko \
    gpio_bank=${BANK} \
    gpio_bits=${BIT0},${BIT1} \
    sample_ms=${SAMPLE_MS}
dmesg | tail -5

echo ""
echo "=== Verificando device file ==="
ls -l ${DEVICE}
sudo chmod a+rw ${DEVICE}

echo ""
echo "=== Leyendo canal 0 (GPIO${BANK}[${BIT0}]) ==="
for i in $(seq 1 5); do
    printf "  Lectura %d: " "$i"
    cat ${DEVICE}
    sleep 0.$(printf '%03d' ${SAMPLE_MS})
done

echo ""
echo "=== Cambiando a canal 1 (GPIO${BANK}[${BIT1}]) ==="
echo 1 > ${DEVICE}

echo ""
echo "=== Leyendo canal 1 (GPIO${BANK}[${BIT1}]) ==="
for i in $(seq 1 5); do
    printf "  Lectura %d: " "$i"
    cat ${DEVICE}
    sleep 0.$(printf '%03d' ${SAMPLE_MS})
done

echo ""
echo "=== Probando índice inválido (debe retornar EINVAL) ==="
echo 5 > ${DEVICE} \
    && echo "ERROR: debería haber fallado" \
    || echo "OK: rechazó índice inválido (EINVAL)"

echo ""
echo "=== Verificando /proc/devices ==="
grep SdC_gpio /proc/devices || echo "(no encontrado)"

echo ""
echo "=== Removiendo módulo ==="
sudo rmmod ${MODULE}
dmesg | tail -3

echo ""
echo "=== Test completo ==="
