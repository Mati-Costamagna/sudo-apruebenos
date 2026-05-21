#include <stdio.h>

int main() {
    int *p = NULL;
    *p = 42;   /* provoca SIGSEGV: escritura en dirección nula */
    return 0;
}
