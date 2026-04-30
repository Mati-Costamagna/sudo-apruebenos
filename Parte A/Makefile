ARCH     = x86_64
EFIINC   = /usr/include/efi
EFILIB   = /usr/lib

CFLAGS = -I$(EFIINC) \
         -I$(EFIINC)/$(ARCH) \
         -I$(EFIINC)/protocol \
         -fno-stack-protector \
         -fpic \
         -fshort-wchar \
         -mno-red-zone \
         -Wall \
         -g

LDFLAGS = -nostdlib \
          -znocombreloc \
          -T $(EFILIB)/elf_x86_64_efi.lds \
          -shared \
          -Bsymbolic \
          -L$(EFILIB) \
          $(EFILIB)/crt0-efi-x86_64.o

all: hello.efi

hello.o: hello.c
	gcc $(CFLAGS) -c hello.c -o hello.o

hello.so: hello.o
	ld $(LDFLAGS) hello.o -o hello.so -lefi -lgnuefi

hello.efi: hello.so
	objcopy -j .text -j .sdata -j .data \
	        -j .dynamic -j .dynsym \
	        -j .rel -j .rela -j .reloc \
	        --target=efi-app-x86_64 \
	        hello.so hello.efi
			