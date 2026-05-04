#include <efi.h>
#include <efilib.h>

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Inicializar la librería GNU-EFI
    InitializeLib(ImageHandle, SystemTable);

    // SOLUCIÓN 1: Usar Print() para evitar el choque de ABI
    Print(L"Iniciando analisis de seguridad...\r\n");

    /*
     * Inyeccion de un software breakpoint (INT3).
     * Si solo querías declarar la variable, tu código estaba bien.
     * Si realmente quieres EJECUTAR el breakpoint para atraparlo con un depurador,
     * debes usar ensamblador. Descomenta la siguiente línea si usas GCC:
     */
     
    // __asm__ __volatile__ ("int3");

    volatile unsigned char code[] = { 0xCC };

    if (code[0] == 0xCC) {
        Print(L"Breakpoint estatico validado en memoria.\r\n");
    }

    // --- SOLUCIÓN 3: Evitar que la VM se reinicie/cuelgue al terminar rápido ---
    Print(L"\r\nPresiona cualquier tecla para finalizar el analisis...\r\n");

    // Limpiar eventos de teclado pendientes
    // NOTA: Para llamar funciones directas, usamos uefi_call_wrapper(Funcion, Numero_De_Argumentos, Arg1, Arg2...)
    uefi_call_wrapper(SystemTable->ConIn->Reset, 2, SystemTable->ConIn, FALSE);

    // Esperar a que se presione una tecla
    EFI_INPUT_KEY Key;
    while (uefi_call_wrapper(SystemTable->ConIn->ReadKeyStroke, 2, SystemTable->ConIn, &Key) == EFI_NOT_READY);

    return EFI_SUCCESS;
}