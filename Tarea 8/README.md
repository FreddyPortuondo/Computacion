# Tarea: biblioteca C con intrinsics SSE2 utilizada desde Python

## Objetivo

Implementar procesamiento SIMD en C, compilar el codigo como objeto compartido `.so`
y llamar a su funcion desde Python mediante `ctypes`. El ejemplo realiza deteccion
de bordes de una imagen, no una simple suma vectorial.

## Requisitos

- Linux x86-64 con soporte SSE2 (presente en los procesadores x86-64 habituales).
- GCC, GNU Make y Python 3.
- No requiere bibliotecas Python externas (tampoco NumPy ni OpenCV).

En Ubuntu/Debian, si faltan herramientas: `sudo apt install build-essential python3`.

## Archivos

- `bordes.c`: implementa `detectar_bordes_sse2` y `detectar_bordes_escalar`.
- `Makefile`: compila `libbordes.so` con `gcc -fPIC -shared -msse2`.
- `ejecutar.py`: carga el `.so`, invoca ambas funciones, comprueba igualdad, mide tiempos y guarda archivos.
- `test_bordes.py`: pruebas de equivalencia y casos extremos.
- `Resultados/`: se crea automaticamente durante la ejecucion.

## Compilar

Desde el directorio de la tarea:

```bash
make
```

El comando equivalente sin Makefile es:

```bash
gcc -O3 -fno-tree-vectorize -std=c11 -Wall -Wextra -Wpedantic -fPIC -msse2 -shared -o libbordes.so bordes.c
```

El indicador `-fno-tree-vectorize` evita que GCC vectorice la referencia escalar automaticamente;
**no desactiva** las intrinsics SSE2 usadas explicitamente.

## Ejecutar y validar

```bash
python3 ejecutar.py
python3 ejecutar.py --ancho 640 --alto 480 --umbral 60 --repeticiones 200
make test
```

Se crean `Resultados/imagen_original.pgm`, `Resultados/bordes_sse2.pgm` y
`Resultados/resultados.txt`, en la carpeta del propio codigo. Los PGM se pueden abrir con GIMP.

## Formula y criterio

Para cada pixel interior `(x,y)` de la imagen original `I`:

```text
Gx = |I(x+1,y) - I(x-1,y)|
Gy = |I(x,y+1) - I(x,y-1)|
M  = min(255, Gx + Gy)
B(x,y) = 255 si M > umbral; si no, 0
```

La imagen `B` es binaria: 255 indica borde, y 0 indica fondo. El contorno
exterior se pone a cero.

Este es un detector basado en gradientes por diferencias centrales y
**no** es el operador Sobel (que utiliza un nucleo de convolucion 3x3).

## Intrinsics usadas

- `_mm_loadu_si128`: lee 16 pixels de 8 bits sin exigir alineacion.
- `_mm_subs_epu8`: resta sin signo con saturacion (permitiendo hallar el valor absoluto).
- `_mm_or_si128`: combina las restas para construir `|a-b|`.
- `_mm_adds_epu8`: suma saturada para magnitud de gradiente (limita a 255).
- `_mm_xor_si128`: ajusta el orden de comparacion unsigned a signed.
- `_mm_cmpgt_epi8`: compara con el umbral, produciendo mascara 0/255.
- `_mm_storeu_si128`: almacena 16 pixels de resultado.

SSE2 opera sobre registros SIMD de 128 bits: **16 pixeles de 8 bits** por lote.
El bucle final procesa 1 a 15 pixeles restantes si el ancho no encaja en lotes de 16.

## Conexion C -> Python

```python
lib = ctypes.CDLL("/ruta/a/libbordes.so")
funcion = lib.detectar_bordes_sse2
funcion.argtypes = [ctypes.POINTER(ctypes.c_uint8),
                    ctypes.POINTER(ctypes.c_uint8),
                    ctypes.c_int, ctypes.c_int, ctypes.c_uint8]
funcion.restype = ctypes.c_int
funcion(entrada, salida, ancho, alto, umbral)
```

No se deben superponer los buffers de entrada y salida. Python reserva ambos buffers,
por lo que C no tiene que liberar memoria ajena.

## Medicion

Se utiliza `time.perf_counter()`, precedido de cinco ejecuciones de calentamiento.
Se reporta el tiempo medio por invocacion, medido **desde Python**, incluido el
pequeno coste de `ctypes`. La aceleracion es:

```text
S = tiempo_medio_escalar / tiempo_medio_SSE2
```

Las cifras dependen del procesador, la frecuencia, el sistema y los parametros.
La igualdad de la salida escalar y SSE2 se comprueba antes de aceptar la medicion.
