/*
 * Biblioteca de procesamiento de imagenes en escala de grises.
 * Deteccion de bordes con intrinsics SSE2 de 128 bits (16 pixeles/ciclo).
 *
 * Compilacion en GNU/Linux x86-64:
 *   gcc -O3 -fno-tree-vectorize -std=c11 -Wall -Wextra -Wpedantic \
 *       -fPIC -msse2 -shared -o libbordes.so bordes.c
 *
 * Algoritmo para cada pixel interior:
 *   Gx = abs(I[y][x+1] - I[y][x-1]);
 *   Gy = abs(I[y+1][x] - I[y-1][x]);
 *   magnitud = min(255, Gx + Gy);
 *   salida = (magnitud > umbral) ? 255 : 0;
 * Los pixeles del borde externo se fijan a cero.
 *
 * IMPORTANTE: la memoria de entrada y salida no debe solaparse.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <emmintrin.h>  /* intrinsics SSE2 */

/* Devuelve 0 si todo va bien; -1 si los parametros no son validos. */
static int validar_argumentos(const uint8_t *entrada, uint8_t *salida,
                              int ancho, int alto)
{
    if (entrada == NULL || salida == NULL || ancho < 3 || alto < 3) {
        return -1;
    }
    if ((size_t)ancho > SIZE_MAX / (size_t)alto) {
        return -1;
    }
    return 0;
}

static uint8_t clasificar_pixel(const uint8_t *entrada,
                               size_t posicion, int ancho,
                               uint8_t umbral)
{
    int gx = (int)entrada[posicion + 1] - (int)entrada[posicion - 1];
    int gy = (int)entrada[posicion + (size_t)ancho]
           - (int)entrada[posicion - (size_t)ancho];
    if (gx < 0) gx = -gx;
    if (gy < 0) gy = -gy;
    int magnitud = gx + gy;
    if (magnitud > 255) magnitud = 255;
    return (magnitud > (int)umbral) ? 255u : 0u;
}

/* Version de referencia sin intrinsics (para validar los resultados). */
int detectar_bordes_escalar(const uint8_t *entrada, uint8_t *salida,
                           int ancho, int alto, uint8_t umbral)
{
    if (validar_argumentos(entrada, salida, ancho, alto) != 0) return -1;
    const size_t total = (size_t)ancho * (size_t)alto;
    memset(salida, 0, total);

    for (int y = 1; y < alto - 1; ++y) {
        for (int x = 1; x < ancho - 1; ++x) {
            size_t pos = (size_t)y * (size_t)ancho + (size_t)x;
            salida[pos] = clasificar_pixel(entrada, pos, ancho, umbral);
        }
    }
    return 0;
}

/* Version SIMD explicita con intrinsics SSE2. */
int detectar_bordes_sse2(const uint8_t *entrada, uint8_t *salida,
                         int ancho, int alto, uint8_t umbral)
{
    if (validar_argumentos(entrada, salida, ancho, alto) != 0) return -1;
    const size_t total = (size_t)ancho * (size_t)alto;
    memset(salida, 0, total);

    /* Para comparar uint8_t sin SSE4.1 convertimos el orden unsigned a signed
       invirtiendo el bit mas significativo antes de _mm_cmpgt_epi8. */
    const __m128i signo = _mm_set1_epi8((char)0x80);
    const __m128i vector_umbral = _mm_xor_si128(
        _mm_set1_epi8((char)umbral), signo);

    for (int y = 1; y < alto - 1; ++y) {
        const size_t desplazamiento = (size_t)y * (size_t)ancho;
        const uint8_t *fila = entrada + desplazamiento;
        const uint8_t *superior = fila - ancho;
        const uint8_t *inferior = fila + ancho;
        uint8_t *destino = salida + desplazamiento;
        int x = 1;

        /* Solo procesar bloques de 16 pixeles completamente interiores. */
        for (; x + 15 < ancho - 1; x += 16) {
            const __m128i izq = _mm_loadu_si128((const __m128i *)(fila + x - 1));
            const __m128i der = _mm_loadu_si128((const __m128i *)(fila + x + 1));
            const __m128i arr = _mm_loadu_si128((const __m128i *)(superior + x));
            const __m128i aba = _mm_loadu_si128((const __m128i *)(inferior + x));

            /* abs(a-b) = saturar(a-b) OR saturar(b-a), para uint8. */
            const __m128i gx = _mm_or_si128(_mm_subs_epu8(der, izq),
                                           _mm_subs_epu8(izq, der));
            const __m128i gy = _mm_or_si128(_mm_subs_epu8(aba, arr),
                                           _mm_subs_epu8(arr, aba));

            const __m128i magnitud = _mm_adds_epu8(gx, gy);
            const __m128i magnitud_con_signo = _mm_xor_si128(magnitud, signo);
            const __m128i mascara = _mm_cmpgt_epi8(magnitud_con_signo,
                                                   vector_umbral);
            _mm_storeu_si128((__m128i *)(destino + x), mascara);
        }

        /* Cola escalar cuando (ancho-2) no es multiplo de 16. */
        for (; x < ancho - 1; ++x) {
            size_t pos = desplazamiento + (size_t)x;
            destino[x] = clasificar_pixel(entrada, pos, ancho, umbral);
        }
    }
    return 0;
}
