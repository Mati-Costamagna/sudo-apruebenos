SECTIONS
{
    . = 0x7c00;             /* El BIOS carga el MBR en 0x7C00 */
    .text :
    {
        *(.text)
        . = 0x1FE;          /* Posicionarse en el byte 510 */
        SHORT(0xAA55)       /* Firma de sector booteable */
    }
}
