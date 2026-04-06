# benchmark_fase1.py
import ctypes
import time

lib = ctypes.CDLL('./libgini.so')
lib.float_to_int.argtypes = [ctypes.c_float]
lib.float_to_int.restype = ctypes.c_int

# Prueba 1: Python nativo
start = time.perf_counter()
for _ in range(100000):
    result = int(42.3)
end = time.perf_counter()
tiempo_python = (end - start) / 100000 * 1_000_000  # μs

# Prueba 2: C mediante ctypes
start = time.perf_counter()
for _ in range(100000):
    result = lib.float_to_int(42.3)
end = time.perf_counter()
tiempo_c = (end - start) / 100000 * 1_000_000  # μs

print(f"Python int():        {tiempo_python:.3f} μs")
print(f"C via ctypes:        {tiempo_c:.3f} μs")
print(f"Overhead de ctypes:  {tiempo_c - tiempo_python:.3f} μs")