#!/bin/bash
set -e

echo "=== Cargando módulo ==="
sudo insmod ../driver/sdec_cdd.ko
dmesg | tail -5

echo ""
echo "=== Verificando device file ==="
ls -l /dev/SdC_cdd

echo ""
echo "=== Leyendo señal 0 (default) ==="
for i in $(seq 1 3); do
    echo -n "  Lectura $i: "
    cat /dev/SdC_cdd
    sleep 1
done

echo ""
echo "=== Cambiando a señal 1 ==="
echo 1 | sudo tee /dev/SdC_cdd > /dev/null

echo ""
echo "=== Leyendo señal 1 ==="
for i in $(seq 1 3); do
    echo -n "  Lectura $i: "
    cat /dev/SdC_cdd
    sleep 1
done

echo ""
echo "=== Probando valor inválido (debe retornar error) ==="
echo 5 | sudo tee /dev/SdC_cdd > /dev/null && echo "ERROR: debería haber fallado" || echo "OK: rechazó valor inválido"

echo ""
echo "=== Removiendo módulo ==="
sudo rmmod sdec_cdd
dmesg | tail -5

echo ""
echo "=== Test completo ==="
