#!/usr/bin/env python3
"""
Benchmark: Comparativa de performance entre Python, C y C+Assembly

Mide el tiempo de ejecución de conversión float→int en tres implementaciones:
1. Python puro (sin ctypes)
2. C (libgini.so - Fase 1)
3. C + Assembly (libgini.so - Fase 2)
"""

import ctypes
import timeit
import sys
import os
from pathlib import Path

# ============================================================================
# Configuration
# ============================================================================

NUM_ITERATIONS = 100000
TEST_VALUES = [42.3, 3.14159, 99.99, 0.5, -15.7, 123.456, 1.1, 2.9]


# ============================================================================
# Implementation 1: Python Puro
# ============================================================================
def python_float_to_int(value):
    """Conversión float→int en Python puro"""
    return int(value)


# ============================================================================
# Implementation 2 & 3: C / C+Assembly
# ============================================================================
def load_c_library(phase="fase2"):
    """Carga la librería compartida de C"""
    lib_path = Path(__file__).parent / phase / "libgini.so"

    if not lib_path.exists():
        print(f"❌ Error: No se encontró {lib_path}")
        print(f"   Asegúrate de compilar primero:")
        print(f"   $ cd {phase} && make all")
        return None

    try:
        lib = ctypes.CDLL(str(lib_path))
        lib.float_to_int.argtypes = [ctypes.c_float]
        lib.float_to_int.restype = ctypes.c_int
        return lib
    except Exception as e:
        print(f"❌ Error al cargar librería: {e}")
        return None


# ============================================================================
# Benchmark Framework
# ============================================================================
def run_benchmark(name, func, values, iterations):
    """Ejecuta benchmark de una función"""
    print(f"\n{'='*70}")
    print(f"Benchmark: {name}")
    print(f"{'='*70}")

    # Warmup
    for _ in range(100):
        func(values[0])

    # Test con cada valor
    times = []
    for val in values:
        def test():
            return func(val)

        elapsed = timeit.timeit(test, number=iterations)
        times.append(elapsed)
        time_per_call = (elapsed / iterations) * 1_000_000  # en microsegundos
        print(f"  f({val:7.3f}) → {func(val):4d} | {time_per_call:8.3f} μs")

    # Resumen
    avg_time = sum(times) / len(times)
    avg_per_call = (avg_time / iterations) * 1_000_000

    print(f"\n  Promedio: {avg_per_call:.3f} μs por llamada")
    print(f"  Total:    {avg_time:.6f} segundos")

    return avg_per_call


# ============================================================================
# Main
# ============================================================================
def main():
    print("\n" + "="*70)
    print("BENCHMARK: Calculadora de Índices GINI")
    print("Comparativa Python vs C vs C+Assembly (x86-64)")
    print("="*70)

    # Cargar librerías
    print("\n[*] Cargando librerías C...")
    lib_fase1 = load_c_library("fase1")
    lib_fase2 = load_c_library("fase2")

    if not lib_fase1 and not lib_fase2:
        print("\n❌ No se pueden cargar las librerías. Abortando.")
        sys.exit(1)

    # Wrapper para fase1 (solo C)
    def c_float_to_int(value):
        return lib_fase1.float_to_int(ctypes.c_float(value))

    # Wrapper para fase2 (C + Assembly)
    def c_asm_float_to_int(value):
        return lib_fase2.float_to_int(ctypes.c_float(value))

    # Ejecutar benchmarks
    results = {}

    results["Python"] = run_benchmark(
        "Python Puro (int())",
        python_float_to_int,
        TEST_VALUES,
        NUM_ITERATIONS
    )

    if lib_fase1:
        results["C (Fase 1)"] = run_benchmark(
            "C Puro (Fase 1)",
            c_float_to_int,
            TEST_VALUES,
            NUM_ITERATIONS
        )

    if lib_fase2:
        results["C+ASM (Fase 2)"] = run_benchmark(
            "C + Assembly x86-64 (Fase 2)",
            c_asm_float_to_int,
            TEST_VALUES,
            NUM_ITERATIONS
        )

    # ========================================================================
    # Resumen Comparativo
    # ========================================================================
    print(f"\n{'='*70}")
    print("RESUMEN COMPARATIVO")
    print(f"{'='*70}")
    print(f"\n{'Implementación':<20} {'Tiempo promedio':>20} {'Speedup relativo':>20}")
    print("-" * 70)

    baseline = results.get("Python")
    for impl, time_us in sorted(results.items(), key=lambda x: x[1]):
        speedup = baseline / time_us if time_us > 0 else 0
        speedup_text = f"1.00x (baseline)" if impl == "Python" else f"{speedup:.2f}x"
        print(f"{impl:<20} {time_us:>15.3f} μs {speedup_text:>20}")

    # Análisis
    print("\n" + "="*70)
    print("ANÁLISIS")
    print("="*70)
    print(f"""
✓ Python actúa como baseline para comparación
✓ C introduce overhead de FFI (ctypes) pero mantiene optimizaciones del compilador
✓ C+Assembly permite acceso directo al procesador con Stack Frame

NOTA: Los tiempos incluyen overhead de ctypes. El overhead real de ctypes
es ~0.5-1 μs por llamada (marshalling de argumentos), lo que significa
que la mayoría del tiempo se gasta en la lógica de la función, no en
la llamada en sí.

Para aplicaciones reales:
- Si el cálculo es trivial → overhead FFI domina
- Si el cálculo es complejo → overhead FFI es negligible
- Assembly es útil cuando se necesita control preciso del CPU
    """)

    print("="*70 + "\n")


if __name__ == "__main__":
    main()
