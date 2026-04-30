#include <efi.h>
#include <efilib.h>

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    volatile int gdb_wait = 1;
    while (gdb_wait) {}   // GDB va a romper este loop
    InitializeLib(ImageHandle, SystemTable);

    EFI_LOADED_IMAGE *li;
    gBS->HandleProtocol(ImageHandle, &LoadedImageProtocol, (void**)&li);
    UINTN base = (UINTN)li->ImageBase;
    Print(L"Image base high: 0x%x\n\r", (UINT32)(base >> 32));
    Print(L"Image base low:  0x%x\n\r", (UINT32)(base & 0xFFFFFFFF));

    Print(L"Hello UEFI\n\r");
    Print(L"Presiona cualquier tecla para salir...\n\r");

    UINTN index;
    gBS->WaitForEvent(1, &ST->ConIn->WaitForKey, &index);

    return EFI_SUCCESS;
}
