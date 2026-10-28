#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define TAM_POBLACION 50
#define NUM_GENERACIONES 1000
#define PROB_CRUCE 0.90
#define PROB_MUTACION 0.01
#define MINIMO -5.12
#define MAXIMO 5.12
#define PI 3.14159265358979323846

#define MASCARA_X 0xFFFFFFFF00000000ULL
#define MASCARA_Y 0x00000000FFFFFFFFULL

typedef struct {
    uint64_t cromosoma;
    double x, y, costo;
} Individuo;

static double azar(void) {
    return (double)rand() / ((double)RAND_MAX + 1.0);
}

static uint32_t gen_aleatorio(void) {
    uint32_t alto = (uint32_t)(rand() & 0xFFFF);
    uint32_t bajo = (uint32_t)(rand() & 0xFFFF);
    return (alto << 16) | bajo;
}

static double convertir(uint32_t gen) {
    return MINIMO + (MAXIMO - MINIMO) * (double)gen / (double)UINT32_MAX;
}

static Individuo evaluar(uint64_t cromosoma) {
    Individuo resultado;
    resultado.cromosoma = cromosoma;
    resultado.x = convertir((uint32_t)(cromosoma >> 32));
    resultado.y = convertir((uint32_t)cromosoma);
    resultado.costo = 20.0 + resultado.x * resultado.x
                            + resultado.y * resultado.y
                            - 10.0 * (cos(2.0 * PI * resultado.x)
                                    + cos(2.0 * PI * resultado.y));
    return resultado;
}

static int comparar(const void *p, const void *q) {
    const Individuo *a = (const Individuo *)p;
    const Individuo *b = (const Individuo *)q;
    return (a->costo > b->costo) - (a->costo < b->costo);
}

static void iniciar(Individuo poblacion[TAM_POBLACION]) {
    for (int i = 0; i < TAM_POBLACION; ++i) {
        uint64_t cromosoma = ((uint64_t)gen_aleatorio() << 32) | gen_aleatorio();
        poblacion[i] = evaluar(cromosoma);
    }
    qsort(poblacion, TAM_POBLACION, sizeof(poblacion[0]), comparar);
}

static void cruzar(uint64_t a, uint64_t b, uint64_t *hijo1, uint64_t *hijo2) {
    *hijo1 = a;
    *hijo2 = b;

    if (azar() < PROB_CRUCE) {
        *hijo1 = (a & MASCARA_X) | (b & MASCARA_Y);
        *hijo2 = (b & MASCARA_X) | (a & MASCARA_Y);
    }
}

static uint64_t mutar(uint64_t cromosoma) {
    if (azar() < PROB_MUTACION) {
        cromosoma ^= 1ULL << (rand() % 64);
    }
    return cromosoma;
}

static void avanzar(Individuo poblacion[TAM_POBLACION]) {
    Individuo candidatos[2 * TAM_POBLACION];
    memcpy(candidatos, poblacion, sizeof(Individuo) * TAM_POBLACION);

    for (int i = 0; i < TAM_POBLACION; i += 2) {
        int p1 = rand() % TAM_POBLACION;
        int p2;
        do {
            p2 = rand() % TAM_POBLACION;
        } while (p1 == p2);

        uint64_t hijo1, hijo2;
        cruzar(poblacion[p1].cromosoma, poblacion[p2].cromosoma,
               &hijo1, &hijo2);

        candidatos[TAM_POBLACION + i] = evaluar(mutar(hijo1));
        if (i + 1 < TAM_POBLACION) {
            candidatos[TAM_POBLACION + i + 1] = evaluar(mutar(hijo2));
        }
    }

    qsort(candidatos, 2 * TAM_POBLACION, sizeof(candidatos[0]), comparar);
    memcpy(poblacion, candidatos, sizeof(Individuo) * TAM_POBLACION);
}

static void imprimir_binario(uint64_t cromosoma) {
    for (int bit = 63; bit >= 0; --bit) {
        putchar((cromosoma >> bit) & 1ULL ? '1' : '0');
        if (bit == 32) putchar('|');
    }
}

static void mostrar_generacion(int numero, const Individuo *mejor) {
    printf("Generacion %d: f(x,y) = %.12f, x = %.12f, y = %.12f\n",
           numero, mejor->costo, mejor->x, mejor->y);
}

int main(void) {
    Individuo poblacion[TAM_POBLACION];
    srand((unsigned int)time(NULL));

    iniciar(poblacion);
    mostrar_generacion(0, &poblacion[0]);

    for (int generacion = 1; generacion <= NUM_GENERACIONES; ++generacion) {
        avanzar(poblacion);
        if (generacion % 100 == 0 || generacion == NUM_GENERACIONES) {
            mostrar_generacion(generacion, &poblacion[0]);
        }
    }

    printf("\nMejor individuo:\nxy = ");
    imprimir_binario(poblacion[0].cromosoma);
    printf("\nx = %.12f\ny = %.12f\nRastrigin(x,y) = %.12f\n",
           poblacion[0].x, poblacion[0].y, poblacion[0].costo);
    return 0;
}
