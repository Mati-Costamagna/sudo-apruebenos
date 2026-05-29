obj-m += drv1.o
obj-m += drv2.o
obj-m += drv3.o
obj-m += drv4.o
obj-m += clipboard.o
obj-m += gpio_cdd.o

# ─── Cross-compilación para BeagleBone Black (ARM Cortex-A8) ─────────────────
#
# Flujo de trabajo:
#   1. Instalar el toolchain en la PC:
#        sudo apt install gcc-arm-linux-gnueabihf
#
#   2. Obtener los headers del kernel de la BBB (hacerlo UNA vez):
#        En la BBB:
#          sudo apt install linux-headers-$(uname -r)
#          tar czf bbb-headers.tar.gz /usr/src/linux-headers-$(uname -r)
#        En la PC:
#          scp debian@<IP_BBB>:~/bbb-headers.tar.gz .
#          tar xzf bbb-headers.tar.gz -C $(HOME)/bbb-kernel/
#
#   3. Ajustar KDIR con la ruta local a esos headers y compilar:
#        make
#
#   4. Transferir el .ko a la BBB:
#        scp gpio_cdd.ko debian@<IP_BBB>:~/

ARCH          := arm
CROSS_COMPILE := arm-linux-gnueabihf-

# Apuntar a los headers del kernel de la BBB copiados en la PC.
# Ajustar esta ruta según donde se extrajeron los headers.
KDIR ?= $(HOME)/bbb-kernel/usr/src/linux-headers-$(shell cat .bbb-kernel-version 2>/dev/null || echo "FALTA_VERSION")

all:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules

clean:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean

# Atajo: guarda la versión del kernel de la BBB para que KDIR se resuelva solo.
# Uso: make set-version BBB=debian@10.42.0.228
set-version:
	ssh $(BBB) "uname -r" > .bbb-kernel-version
	@echo "Versión guardada: $$(cat .bbb-kernel-version)"

# Atajo: transfiere todos los .ko generados a la BBB.
# Uso: make deploy BBB=debian@10.42.0.228
deploy:
	scp *.ko $(BBB):~/gpio_driver/
