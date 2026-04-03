import requests
import ctypes

API_URL = "https://api.worldbank.org/v2/en/country/all/indicator/SI.POV.GINI?format=json&date=2011:2020&per_page=32500&page=1&country=%22Argentina%22"

def get_gini_data():
    try:
        response = requests.get(API_URL)
        response.raise_for_status()
        return response.json()
    except Exception as e:
        print(f"Error al obtener datos: {e}")
        return None

def cargar_libreria():
    
    lib = ctypes.CDLL('./libgini.so')

    # Definimos los tipos de entrada y salida para que coincidan con C 
    lib.float_to_int.argtypes = [ctypes.c_float]
    lib.float_to_int.restype = ctypes.c_int

    lib.sumar_uno.argtypes = [ctypes.c_int]
    lib.sumar_uno.restype = ctypes.c_int
    
    return lib

def procesar():
    data = get_gini_data()
    lib = cargar_libreria()

    if data and lib:
        #Filtramos los datos para que solo sean de "Argentina" y tengan valor
        datos_argentina = [
            entry for entry in data[1] 
            if entry.get('country', {}).get('value') == "Argentina" and entry.get('value') is not None
        ]

        #Ordenamos por año (de más reciente a más antiguo)
        datos_argentina.sort(key=lambda x: x['date'], reverse=True)

        #Tomamos solo los primeros 5 (los más recientes)
        #top_5 = datos_argentina[:5]

        print(f"\nResultados para: Argentina (Últimos 5 años disponibles)")
        print(f"{'AÑO':<6} | {'GINI ORIGINAL':<15} | {'ENTERO':<12} | {'SUMA +1':<10}")
        print("-" * 55)
        
        for entry in top_5:
            valor_float = entry.get('value')
            anio = entry.get('date')

            gini_int = lib.float_to_int(valor_float)
            
            gini_final = lib.sumar_uno(gini_int)

            print(f"{anio:<6} | {valor_float:<15.2f} | {gini_int:<12} | {gini_final:<10}")

if __name__ == "__main__":
    procesar()

