#include <efi.h>
#include <efilib.h>

EFI_STATUS //devuelve codigo exito o error
EFIAPI //convencion de llamadas que usa UEFI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    InitializeLib(ImageHandle, SystemTable); //inicializa gnu-file y system table

    Print(L"Hello, UEFI World!\n\r");
    Print(L"Presiona cualquier tecla para salir...\n\r");

    UINTN index;
    gBS->WaitForEvent(1, &ST->ConIn->WaitForKey, &index); //espera a que el usuario presione una tecla

    return EFI_SUCCESS;
}
