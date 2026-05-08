CC      = gcc
LD      = ld
OBJCOPY = objcopy

EFI_INCLUDE = /usr/include/efi
ARCH        = x86_64

CFLAGS = \
    -I$(EFI_INCLUDE) \
    -I$(EFI_INCLUDE)/$(ARCH) \
    -fno-stack-protector \
    -fpic \
    -fshort-wchar

CFLAGS_DEBUG = $(CFLAGS) -g -O0 -DDEBUG_WAIT

LDFLAGS = \
    -nostdlib \
    -T /usr/lib/elf_x86_64_efi.lds \
    -shared \
    -Bsymbolic \
    -L/usr/lib \
    /usr/lib/crt0-efi-x86_64.o \
    -lefi \
    -lgnuefi

SECTIONS = \
    -j .text \
    -j .sdata \
    -j .data \
    -j .dynamic \
    -j .dynsym \
    -j .rel \
    -j .rela \
    -j .reloc

DISK_DIR = $(HOME)/UEFI_disk
OVMF     = /usr/share/ovmf/OVMF.fd

.PHONY: all debug run run-debug text-offset clean

all: aplicacion.efi

# --- build normal ---
aplicacion.o: aplicacion.c
	$(CC) $(CFLAGS) -c -o $@ $<

aplicacion.so: aplicacion.o
	$(LD) $(LDFLAGS) $< -o $@

aplicacion.efi: aplicacion.so
	$(OBJCOPY) $(SECTIONS) --target=efi-app-$(ARCH) $< $@

# --- build debug (con símbolos + bucle de espera) ---
aplicacion_debug.o: aplicacion.c
	$(CC) $(CFLAGS_DEBUG) -c -o $@ $<

aplicacion_debug.so: aplicacion_debug.o
	$(LD) $(LDFLAGS) $< -o $@

aplicacion_debug.efi: aplicacion_debug.so
	$(OBJCOPY) $(SECTIONS) --target=efi-app-$(ARCH) $< $@

# Muestra el offset de .text en el .so para usar en add-symbol-file
text-offset: aplicacion_debug.so
	@echo "Offset de la seccion .text (usar en add-symbol-file: IMAGE_BASE + offset):"
	@objdump -h $< | awk '/\.text/{printf "  0x%s\n", $$4}'

run: aplicacion.efi
	mkdir -p $(DISK_DIR)
	cp aplicacion.efi $(DISK_DIR)/
	qemu-system-x86_64 \
	    -drive if=pflash,format=raw,readonly=on,file=$(OVMF) \
	    -drive format=raw,file=fat:rw:$(DISK_DIR) \
	    -net none \
	    -display gtk

# Lanza QEMU con servidor GDB habilitado y CPU pausada al inicio
run-debug: aplicacion_debug.efi
	mkdir -p $(DISK_DIR)
	cp aplicacion_debug.efi $(DISK_DIR)/aplicacion.efi
	@echo "QEMU iniciando con servidor GDB en localhost:1234 (CPU pausada)."
	@echo "En otra terminal: gdb -x debug.gdb"
	qemu-system-x86_64 \
	    -drive if=pflash,format=raw,readonly=on,file=$(OVMF) \
	    -drive format=raw,file=fat:rw:$(DISK_DIR) \
	    -net none \
	    -display gtk \
	    -s \
	    -S

clean:
	rm -f aplicacion.o aplicacion.so aplicacion.efi
	rm -f aplicacion_debug.o aplicacion_debug.so aplicacion_debug.efi
