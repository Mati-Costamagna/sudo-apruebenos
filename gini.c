#include "gini.h"

int float_to_int(float value) {
    // Llamamos a la rutina en assembler pasando los dummies
    // para forzar que 'value' viaje por el stack (15to argumento)
    return asm_float_to_int(0,0,0,0,0,0, 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0, value);
}

int sumar_uno(int value) {
    // Llamamos a la rutina en assembler pasando los dummies
    // para forzar que 'value' viaje por el stack (7mo argumento)
    return asm_sumar_uno(0,0,0,0,0,0, value);
}
