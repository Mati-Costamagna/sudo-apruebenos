#ifndef GINI_H
#define GINI_H

// Funciones exportadas a Python (interfaz simple)
// Coinciden con ctypes: c_float -> float, c_int -> int
int float_to_int(float gini);
int sumar_uno(int gini);

// Funciones en assembler: parametros forzados por stack
// 6 dummies enteros llenan %rdi-%r9, 8 dummies float llenan %xmm0-%xmm7
// El 9no float va por stack
extern int asm_float_to_int(long d1, long d2, long d3,
                            long d4, long d5, long d6,
                            double f1, double f2, double f3,
                            double f4, double f5, double f6,
                            double f7, double f8,
                            float gini_value);

// 6 dummies enteros llenan %rdi-%r9, el 7mo entero va por stack
extern int asm_sumar_uno(long d1, long d2, long d3,
                         long d4, long d5, long d6,
                         int gini_int);

#endif
