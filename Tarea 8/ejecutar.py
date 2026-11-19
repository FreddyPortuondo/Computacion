#!/usr/bin/env python3
"""Carga una biblioteca compartida de C con ctypes y genera bordes de una imagen.

No requiere NumPy, Pillow ni otros paquetes externos.
"""

import argparse
import ctypes
from pathlib import Path
from time import perf_counter

CARPETA = Path(__file__).resolve().parent
BIBLIOTECA = CARPETA / "libbordes.so"


def cargar_biblioteca():
    if not BIBLIOTECA.is_file():
        raise SystemExit(
            f"No se encuentra {BIBLIOTECA.name}. Primero ejecuta 'make' "
            f"en {CARPETA}."
        )
    lib = ctypes.CDLL(str(BIBLIOTECA))
    firma = [ctypes.POINTER(ctypes.c_uint8),
             ctypes.POINTER(ctypes.c_uint8),
             ctypes.c_int, ctypes.c_int, ctypes.c_uint8]
    for nombre in ("detectar_bordes_escalar", "detectar_bordes_sse2"):
        funcion = getattr(lib, nombre)
        funcion.argtypes = firma
        funcion.restype = ctypes.c_int
    return lib


def crear_imagen(ancho, alto):
    """Imagen sintetica: rectangulos, rejilla y diagonal, sin librerias externas."""
    img = bytearray(ancho * alto)
    for y in range(alto):
        for x in range(ancho):
            base = 25 + (x // 45 % 2) * 12 + (y // 35 % 2) * 8
            rectangulo1 = ancho // 10 <= x < ancho // 3 and alto // 8 <= y < alto // 2
            rectangulo2 = ancho // 2 <= x < 9 * ancho // 10 and alto // 2 <= y < 7 * alto // 8
            diagonal = abs(y - (x * alto // ancho)) <= max(1, alto // 300)
            img[y * ancho + x] = 215 if rectangulo1 else (145 if rectangulo2 else (250 if diagonal else base))
    return img


def escribir_pgm(ruta, ancho, alto, pixeles):
    """Formato PGM binario P5, compatible con GIMP y ImageMagick."""
    with ruta.open("wb") as archivo:
        archivo.write(f"P5\n{ancho} {alto}\n255\n".encode("ascii"))
        archivo.write(bytes(pixeles))


def llamar(funcion, imagen, salida, ancho, alto, umbral):
    estado = funcion(imagen, salida, ancho, alto, umbral)
    if estado != 0:
        raise RuntimeError(f"La funcion en C termino con error {estado}")


def medir(funcion, imagen, salida, ancho, alto, umbral, repeticiones):
    # Calentamiento antes de medir para reducir variaciones por primera ejecucion.
    for _ in range(5):
        llamar(funcion, imagen, salida, ancho, alto, umbral)
    inicio = perf_counter()
    for _ in range(repeticiones):
        llamar(funcion, imagen, salida, ancho, alto, umbral)
    return (perf_counter() - inicio) * 1000 / repeticiones


def main():
    parser = argparse.ArgumentParser(description="Deteccion de bordes C SSE2 desde Python con ctypes")
    parser.add_argument("--ancho", type=int, default=1024)
    parser.add_argument("--alto", type=int, default=768)
    parser.add_argument("--umbral", type=int, default=75)
    parser.add_argument("--repeticiones", type=int, default=100)
    args = parser.parse_args()
    if args.ancho < 3 or args.alto < 3:
        parser.error("El ancho y el alto deben ser >= 3")
    if not 0 <= args.umbral <= 255:
        parser.error("El umbral debe estar entre 0 y 255")
    if args.repeticiones < 1:
        parser.error("Las repeticiones deben ser >= 1")

    lib = cargar_biblioteca()
    pixeles = crear_imagen(args.ancho, args.alto)
    total = len(pixeles)
    TipoVector = ctypes.c_uint8 * total
    entrada = TipoVector.from_buffer_copy(pixeles)
    salida_escalar = TipoVector()
    salida_sse2 = TipoVector()

    llamar(lib.detectar_bordes_escalar, entrada, salida_escalar,
           args.ancho, args.alto, args.umbral)
    llamar(lib.detectar_bordes_sse2, entrada, salida_sse2,
           args.ancho, args.alto, args.umbral)

    if bytes(salida_escalar) != bytes(salida_sse2):
        discrepancias = sum(a != b for a, b in zip(salida_escalar, salida_sse2))
        raise AssertionError(f"Las versiones escalar y SSE2 difieren en {discrepancias} pixeles")

    tiempo_escalar = medir(lib.detectar_bordes_escalar, entrada, salida_escalar,
                           args.ancho, args.alto, args.umbral, args.repeticiones)
    tiempo_sse2 = medir(lib.detectar_bordes_sse2, entrada, salida_sse2,
                        args.ancho, args.alto, args.umbral, args.repeticiones)
    aceleracion = tiempo_escalar / tiempo_sse2 if tiempo_sse2 else float("inf")
    bordes = bytes(salida_sse2).count(255)

    carpeta_resultados = CARPETA / "Resultados"
    carpeta_resultados.mkdir(exist_ok=True)
    escribir_pgm(carpeta_resultados / "imagen_original.pgm", args.ancho, args.alto, entrada)
    escribir_pgm(carpeta_resultados / "bordes_sse2.pgm", args.ancho, args.alto, salida_sse2)
    informe = (f"Resolucion: {args.ancho} x {args.alto}\n"
               f"Umbral: {args.umbral}\n"
               f"Repeticiones: {args.repeticiones}\n"
               f"Pixeles de borde detectados: {bordes}\n"
               f"Comparacion escalar/SSE2: IDENTICA\n"
               f"Tiempo medio escalar: {tiempo_escalar:.5f} ms\n"
               f"Tiempo medio SSE2: {tiempo_sse2:.5f} ms\n"
               f"Aceleracion (escalar/SSE2): {aceleracion:.3f}x\n")
    (carpeta_resultados / "resultados.txt").write_text(informe, encoding="utf-8")
    print(informe)
    print(f"Archivos guardados en: {carpeta_resultados}")


if __name__ == "__main__":
    main()
