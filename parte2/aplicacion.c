#include <efi.h>
#include <efilib.h>

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    SystemTable->ConOut->OutputString(
        SystemTable->ConOut,
        L"Iniciando analisis de seguridad...\r\n"
    );

    /*
     * Inyeccion de un software breakpoint (INT3).
     * 0xCC es el opcode x86 de INT3. En Ghidra aparece como -52 (signed byte)
     * porque 0xCC = 204 decimal = -52 en complemento a dos de 8 bits.
     */
    unsigned char code[] = { 0xCC };

    if (code[0] == 0xCC) {
        SystemTable->ConOut->OutputString(
            SystemTable->ConOut,
            L"Breakpoint estatico alcanzado.\r\n"
        );
    }

    return EFI_SUCCESS;
}
