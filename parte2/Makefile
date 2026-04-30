CC      = gcc
LD      = ld
OBJCOPY = objcopy

EFI_INCLUDE = /usr/include/efi
ARCH        = x86_64

CFLAGS = \
    -I$(EFI_INCLUDE) \
    -I$(EFI_INCLUDE)/$(ARCH) \
    -I$(EFI_INCLUDE)/protocol \
    -fpic \
    -ffreestanding \
    -fno-stack-protector \
    -fno-strict-aliasing \
    -fshort-wchar \
    -mno-red-zone \
    -maccumulate-outgoing-args \
    -Wall \
    -O0

LDFLAGS = \
    -shared \
    -Bsymbolic \
    -L/usr/lib \
    -L/usr/lib/efi \
    -T /usr/lib/elf_x86_64_efi.lds \
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
    -j .rel.* \
    -j .rela.* \
    -j .reloc

.PHONY: all clean check-deps

all: aplicacion.efi

check-deps:
	@echo "Verificando dependencias..."
	@pkg-config --exists gnu-efi 2>/dev/null || \
		(dpkg -l gnu-efi 2>/dev/null | grep -q "^ii" && echo "gnu-efi instalado") || \
		(echo "ERROR: gnu-efi no encontrado. Instalar con: sudo apt install gnu-efi" && exit 1)
	@echo "OK"

aplicacion.o: aplicacion.c
	$(CC) $(CFLAGS) -c -o $@ $<

aplicacion.so: aplicacion.o
	$(LD) $(LDFLAGS) aplicacion.o -o $@

aplicacion.efi: aplicacion.so
	$(OBJCOPY) $(SECTIONS) --target=efi-app-$(ARCH) $< $@

clean:
	rm -f aplicacion.o aplicacion.so aplicacion.efi

info: aplicacion.efi
	@echo "=== file (formato PE/COFF del .efi) ==="
	file aplicacion.efi
	@echo ""
	@echo "=== objdump -f (cabecera PE/COFF) ==="
	objdump -f aplicacion.efi
	@echo ""
	@echo "=== objdump -p (detalles PE: secciones, subsistema) ==="
	objdump -p aplicacion.efi
	@echo ""
	@echo "=== readelf -h (cabecera ELF del .so intermedio) ==="
	readelf -h aplicacion.so
