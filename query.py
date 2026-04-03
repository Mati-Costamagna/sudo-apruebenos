import requests
import ctypes

## Etapa 1 - Consulta a API

API_URL = ("https://api.worldbank.org/v2/en/country/all/indicator/SI.POV.GINI?format=json&date=2011:2020&per_page=32500&page=1&country=%22Argentina%22")

def get_gini_data():
    try:
        response = requests.get(API_URL)
        response.raise_for_status()  # Verificar si la solicitud fue exitosa
        data = response.json()
        return data
    except requests.exceptions.RequestException as e:
        print(f"Error fetching Gini data: {e}")
        return None


def parse_gini_data(data):
    results = []
    for entry in data[1]:  # El primer elemento es metadata, el segundo es la lista de datos
        value = entry.get('value')
        if value is None:
            continue
        
        pais = entry.get('country', {})
        results.append({
            'pais': pais.get('value', 'N/A'),
            'codigo_pais': pais.get('id', 'N/A'),
            'anio': entry.get('date', 'N/A'),
            'gini': float(value),
        })

    results.sort(key=lambda x: (x['pais'], x['anio']))  # Ordenar por nombre del país y año
    return results

def mostrar_datos(datos, filtro_pais=None):
    for dato in datos:
        if filtro_pais and dato['pais'] != filtro_pais:
            continue
        print(f"País: {dato['pais']}, Código: {dato['codigo_pais']}, Año: {dato['anio']}, Gini: {dato['gini']}")

    if not datos:
        print("No se encontraron datos para el país especificado.")
    
def cargar_lib_c():
    try:
        lib = ctypes.CDLL('./libgini.so')
    except OSError as e:
        print(f"Error loading C library: {e}")
        return None
    
    lib.float_to_int.argtypes = [ctypes.c_float]
    lib.float_to_int.restype = ctypes.c_int

    lib.sumar_uno.argtypes = [ctypes.c_int]
    lib.sumar_uno.restype = ctypes.c_int

    return lib

def procesar_datos_c(lib, datos, filtro_pais=None):
    for dato in datos:
        if filtro_pais and dato['pais'] != filtro_pais:
            continue
        
        gini_int = lib.float_to_int(ctypes.c_float(dato['gini']))
        gini_sumado = lib.sumar_uno(ctypes.c_int(gini_int))
        
        print(f"País: {dato['pais']}, Año: {dato['anio']}, Gini (int): {gini_int}, Gini + 1: {gini_sumado}")



if __name__ == "__main__":
    gini_data = get_gini_data()
    if gini_data:
        lib = cargar_lib_c()
        parsed_data = []
        if lib:
            parsed_data = parse_gini_data(gini_data)
            procesar_datos_c(lib, parsed_data, filtro_pais="Argentina")