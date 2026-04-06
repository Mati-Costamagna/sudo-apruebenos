# Cargar simbolos de la shared library
set auto-load safe-path /home/matias/Documents/Facultad/SdC

# Breakpoints en las funciones C y ASM
break float_to_int
break asm_float_to_int
break sumar_uno
break asm_sumar_uno

# Mostrar registros relevantes al llegar a un breakpoint
define hook-stop
  echo \n--- Registros ---\n
  info registers rdi rsi rdx rcx r8 r9 eax
  echo \n--- Stack (rbp+0x10) ---\n
  x/f $rbp+0x10
  echo \n
end
