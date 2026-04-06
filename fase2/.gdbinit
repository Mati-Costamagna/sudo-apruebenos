# Configuración para carga dinámica
set breakpoint pending on
set auto-load safe-path /

# Breakpoints en las funciones de la librería compartida
break float_to_int
break asm_float_to_int
break sumar_uno
break asm_sumar_uno

# Hook para mostrar información automáticamente al frenar
define hook-stop
  echo \n--- Registros (Argumentos/Retorno) ---\n
  info registers rdi rsi xmm0 eax
  echo \n--- Stack Frame Actual ($rbp) ---\n
  echo 0x0(%rbp): Old RBP  -> 
  x/gx $rbp
  echo 0x8(%rbp): Ret Addr -> 
  x/gx $rbp+8
  echo 0x10(%rbp): Argument -> 
  x/f $rbp+16
  echo \n
end
