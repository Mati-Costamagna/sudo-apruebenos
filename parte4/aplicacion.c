#include <efi.h>
#include <efilib.h>

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

#ifdef DEBUG_WAIT
    // Bucle de espera para depuración con GDB.
    // Conectar GDB, pausar con Ctrl+C y ejecutar: set var waiting=0
    volatile int waiting = 1;
    while (waiting) {}
#endif

    Print(L"Iniciando analisis de seguridad...\r\n");

    volatile unsigned char code[] = { 0xCC };

    if (code[0] == 0xCC) {
        Print(L"Breakpoint estatico validado en memoria.\r\n");
    }

    Print(L"\r\nPresiona cualquier tecla para finalizar el analisis...\r\n");

    uefi_call_wrapper(SystemTable->ConIn->Reset, 2, SystemTable->ConIn, FALSE);

    EFI_INPUT_KEY Key;
    while (uefi_call_wrapper(SystemTable->ConIn->ReadKeyStroke, 2, SystemTable->ConIn, &Key) == EFI_NOT_READY);

    return EFI_SUCCESS;
}
