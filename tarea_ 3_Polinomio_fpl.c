#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <immintrin.h>

#define N_COEFICIENTES 10000
#define REPETICIONES 100000

/* Metodo de Horner secuencial usando precision simple. */
static float horner(float x, const float *coef, size_t n) {
    float acumulador = 0.0f;
    for (size_t i = 0; i < n; ++i)
        acumulador = (acumulador + coef[i]) * x;
    return acumulador;
}

/* AVX de 256 bits: 8 valores float en paralelo. */
static float horner_avx(float x, const float *coef, size_t n) {
    if (n == 0) return 0.0f;
    if (n % 8 != 0) return horner(x, coef, n);

    const size_t bloques = n / 8;
    const float x2 = x * x;
    const float x3 = x2 * x;
    const float x4 = x2 * x2;
    const float x5 = x4 * x;
    const float x6 = x4 * x2;
    const float x7 = x4 * x3;
    const float x8 = x4 * x4;

    const __m256 potencia = _mm256_set1_ps(x8);
    __m256 acumulador = _mm256_setzero_ps();

    for (size_t i = 0; i < bloques; ++i) {
        acumulador = _mm256_add_ps(acumulador, _mm256_load_ps(coef + 8 * i));
        if (i + 1 < bloques)
            acumulador = _mm256_mul_ps(acumulador, potencia);
    }

    float partes[8];
    _mm256_storeu_ps(partes, acumulador);
    return partes[7] * x  + partes[6] * x2
         + partes[5] * x3 + partes[4] * x4
         + partes[3] * x5 + partes[2] * x6
         + partes[1] * x7 + partes[0] * x8;
}

/* Cronometro en nanosegundos; evita perdida de precision en las restas. */
static uint64_t tiempo_ns(void) {
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    return (uint64_t)t.tv_sec * UINT64_C(1000000000) + (uint64_t)t.tv_nsec;
}

int main(int argc, char *argv[]) {
    float x = 1.1f;
    long repeticiones = REPETICIONES;

    if (argc > 3) {
        fprintf(stderr, "Uso: %s [x] [repeticiones]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc >= 2) {
        char *fin;
        errno = 0;
        x = strtof(argv[1], &fin);
        if (errno || fin == argv[1] || *fin != '\0' || !isfinite(x)) {
            fprintf(stderr, "Valor de x no valido.\n");
            return EXIT_FAILURE;
        }
    }
    if (argc == 3) {
        char *fin;
        errno = 0;
        repeticiones = strtol(argv[2], &fin, 10);
        if (errno || fin == argv[2] || *fin != '\0' || repeticiones <= 0) {
            fprintf(stderr, "Numero de repeticiones no valido.\n");
            return EXIT_FAILURE;
        }
    }

    float *coeficientes = _mm_malloc(N_COEFICIENTES * sizeof(*coeficientes), 32);
    if (coeficientes == NULL) {
        fprintf(stderr, "No se pudo reservar memoria.\n");
        return EXIT_FAILURE;
    }

    srand((unsigned int)time(NULL));
    for (size_t i = 0; i < N_COEFICIENTES; ++i) {
        coeficientes[i] = (float)(rand() % 1000) / 1000.0f;
        if (i < 10) printf("coef[%zu] = %.3f\n", i, coeficientes[i]);
    }

    float resultado_normal = horner(x, coeficientes, N_COEFICIENTES);
    float resultado_avx = horner_avx(x, coeficientes, N_COEFICIENTES);
    printf("\nx = %.6f\nHorner:     %.9g\nHorner AVX: %.9g\n",
           x, resultado_normal, resultado_avx);

    if (isfinite(resultado_normal) && isfinite(resultado_avx)) {
        float error = fabsf(resultado_normal - resultado_avx);
        printf("Error absoluto: %.8g\n", error);
    } else {
        puts("Aviso: resultado no finito; use, por ejemplo, x=0.9 para comparar resultados.");
    }

    /* Volatile impide que el compilador elimine las evaluaciones repetidas. */
    volatile float x_medicion = x;
    volatile float resultado_uso;
    uint64_t inicio = tiempo_ns();
    for (long j = 0; j < repeticiones; ++j)
        resultado_uso = horner(x_medicion, coeficientes, N_COEFICIENTES);
    float tiempo_normal = (float)(tiempo_ns() - inicio) / 1000000000.0f;

    inicio = tiempo_ns();
    for (long j = 0; j < repeticiones; ++j)
        resultado_uso = horner_avx(x_medicion, coeficientes, N_COEFICIENTES);
    float tiempo_avx = (float)(tiempo_ns() - inicio) / 1000000000.0f;
    (void)resultado_uso;

    printf("\nTiempo Horner:     %.6f s\n", tiempo_normal);
    printf("Tiempo Horner AVX: %.6f s\n", tiempo_avx);
    if (tiempo_avx > 0.0f)
        printf("Aceleracion:       %.3f veces\n", tiempo_normal / tiempo_avx);

    _mm_free(coeficientes);
    return EXIT_SUCCESS;
}
