obj-m += drv1.o
obj-m += drv2.o
obj-m += drv3.o
obj-m += drv4.o
obj-m += clipboard.o
obj-m += gpio_cdd.o

# ─── Cross-compilación para BeagleBone Black (ARM Cortex-A8) ─────────────────
#
# Flujo de trabajo completo:
#
#   1. Instalar el toolchain en la PC (una sola vez):
#        sudo apt install gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf
#
#   2. Obtener los headers del kernel de la BBB (una sola vez):
#        En la BBB:
#          sudo apt install linux-headers-$(uname -r)
#          tar czf bbb-headers.tar.gz /usr/src/linux-headers-$(uname -r)
#        En la PC:
#          scp debian@<IP_BBB>:~/bbb-headers.tar.gz .
#          tar xzf bbb-headers.tar.gz -C $(HOME)/bbb-kernel/
#
#   3. Guardar la versión del kernel de la BBB:
#        make set-version BBB=debian@10.42.0.228
#
#   4. Compilar en la PC apuntando a ARM:
#        make
#
#   5. Transferir módulos y app a la BBB:
#        make deploy-all BBB=debian@10.42.0.228
#
#   6. Configurar pines y cargar el módulo en la BBB:
#        make setup-bbb BBB=debian@10.42.0.228
#
#   7. Arrancar el servidor web en la BBB:
#        make run-server BBB=debian@10.42.0.228
#        → abrir en PC: http://<IP_BBB>:8080

ARCH          := arm
CROSS_COMPILE := arm-linux-gnueabihf-

# Ruta a los headers del kernel de la BBB copiados en la PC.
# Se resuelve automáticamente desde .bbb-kernel-version (make set-version).
KDIR ?= $(HOME)/bbb-kernel/usr/src/linux-headers-$(shell cat .bbb-kernel-version 2>/dev/null || echo "FALTA_VERSION")

# Directorio remoto en la BBB donde se despliegan los binarios.
BBB_DIR ?= ~/gpio_driver

all:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules

clean:
	make -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean

# Guarda la versión del kernel de la BBB para que KDIR se resuelva solo.
# Uso: make set-version BBB=debian@10.42.0.228
set-version:
	ssh $(BBB) "uname -r" > .bbb-kernel-version
	@echo "Versión guardada: $$(cat .bbb-kernel-version)"

# Copia los archivos de configuración generados desde la BBB.
# Necesario cuando 'make' falla con "autoconf.h or auto.conf are missing".
# Uso: make prepare-headers BBB=debian@10.42.0.228
prepare-headers:
	$(eval KVER := $(shell cat .bbb-kernel-version 2>/dev/null || (echo "ERROR: ejecutar make set-version primero" && exit 1)))
	$(eval REMOTE_KDIR := /usr/src/linux-headers-$(KVER))
	@echo "Copiando archivos de configuración desde $(BBB):$(REMOTE_KDIR) ..."
	mkdir -p $(KDIR)/include/generated $(KDIR)/include/config
	scp $(BBB):$(REMOTE_KDIR)/include/generated/autoconf.h \
	    $(KDIR)/include/generated/autoconf.h
	scp $(BBB):$(REMOTE_KDIR)/include/config/auto.conf \
	    $(KDIR)/include/config/auto.conf
	@# Algunos headers también necesitan este directorio para scripts Kbuild
	-scp $(BBB):$(REMOTE_KDIR)/include/config/tristate.conf \
	     $(KDIR)/include/config/tristate.conf 2>/dev/null || true
	@echo "Listo. Intentar 'make' nuevamente."

# Transfiere los módulos .ko a la BBB.
# Uso: make deploy BBB=debian@10.42.0.228
deploy:
	ssh $(BBB) "mkdir -p $(BBB_DIR)"
	scp *.ko $(BBB):$(BBB_DIR)/

# Transfiere la aplicación web a la BBB.
# Uso: make deploy-app BBB=debian@10.42.0.228
deploy-app:
	ssh $(BBB) "mkdir -p $(BBB_DIR)"
	scp app/signal_server.py $(BBB):$(BBB_DIR)/

# Transfiere módulos y aplicación en un solo paso.
# Uso: make deploy-all BBB=debian@10.42.0.228
deploy-all: deploy deploy-app

# Configura los pines GPIO en la BBB y carga gpio_cdd.ko.
# Uso: make setup-bbb BBB=debian@10.42.0.228
setup-bbb:
	ssh -t $(BBB) "sudo config-pin P9_12 gpio && sudo config-pin P9_14 gpio && \
	               sudo insmod $(BBB_DIR)/gpio_cdd.ko && \
	               sudo chmod a+rw /dev/SdC_gpio && \
	               dmesg | tail -3"

# Arranca el servidor web en la BBB (en segundo plano).
# Uso: make run-server BBB=debian@10.42.0.228
run-server:
	ssh $(BBB) "nohup python3 $(BBB_DIR)/signal_server.py > $(BBB_DIR)/server.log 2>&1 &"
	@echo "Servidor iniciado. Abrir: http://$(shell echo $(BBB) | cut -d@ -f2):8080"

.PHONY: all clean set-version prepare-headers deploy deploy-app deploy-all setup-bbb run-server
