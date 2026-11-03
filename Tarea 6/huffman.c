#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#define SIMBOLOS 256
#define MAX_NODOS (2 * SIMBOLOS - 1)

/* Un símbolo es un byte (0..255): funciona también con archivos UTF-8. */
typedef struct NodoHuffman {
    int simbolo;                 /* -1 indica nodo interno */
    int menor_simbolo;           /* desempate determinista del montículo */
    uint64_t frecuencia;
    struct NodoHuffman *izq, *der;
} NodoHuffman;

typedef struct {
    NodoHuffman nodos[MAX_NODOS];
    size_t usados;
    NodoHuffman *heap[SIMBOLOS];
    size_t nheap;
    char codigos[SIMBOLOS][SIMBOLOS];
} CompresorHuffman;

typedef struct {
    unsigned char byte;
    unsigned int bits;
} EstadoBits;

static NodoHuffman *nuevo_nodo(CompresorHuffman *c, int simbolo,
                               uint64_t frecuencia, NodoHuffman *izq,
                               NodoHuffman *der)
{
    if (c->usados == MAX_NODOS) return NULL;
    NodoHuffman *n = &c->nodos[c->usados++];
    n->simbolo = simbolo;
    n->frecuencia = frecuencia;
    n->izq = izq;
    n->der = der;
    n->menor_simbolo = simbolo >= 0 ? simbolo :
        (izq->menor_simbolo < der->menor_simbolo ?
         izq->menor_simbolo : der->menor_simbolo);
    return n;
}

static int menor(const NodoHuffman *a, const NodoHuffman *b)
{
    return a->frecuencia < b->frecuencia ||
           (a->frecuencia == b->frecuencia &&
            a->menor_simbolo < b->menor_simbolo);
}

static void heap_insertar(CompresorHuffman *c, NodoHuffman *n)
{
    size_t i = c->nheap++;
    while (i > 0 && menor(n, c->heap[(i - 1) / 2])) {
        c->heap[i] = c->heap[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    c->heap[i] = n;
}

static NodoHuffman *heap_extraer(CompresorHuffman *c)
{
    NodoHuffman *minimo = c->heap[0];
    NodoHuffman *ultimo = c->heap[--c->nheap];
    size_t i = 0;
    if (c->nheap == 0) return minimo;
    while (2 * i + 1 < c->nheap) {
        size_t hijo = 2 * i + 1;
        if (hijo + 1 < c->nheap && menor(c->heap[hijo + 1], c->heap[hijo]))
            ++hijo;
        if (!menor(c->heap[hijo], ultimo)) break;
        c->heap[i] = c->heap[hijo];
        i = hijo;
    }
    c->heap[i] = ultimo;
    return minimo;
}

static NodoHuffman *construir_arbol(CompresorHuffman *c,
                                    const uint64_t frecuencias[SIMBOLOS])
{
    for (int s = 0; s < SIMBOLOS; ++s)
        if (frecuencias[s])
            heap_insertar(c, nuevo_nodo(c, s, frecuencias[s], NULL, NULL));

    while (c->nheap > 1) {
        NodoHuffman *a = heap_extraer(c);
        NodoHuffman *b = heap_extraer(c);
        NodoHuffman *padre = nuevo_nodo(c, -1, a->frecuencia + b->frecuencia, a, b);
        heap_insertar(c, padre);
    }
    return c->nheap ? heap_extraer(c) : NULL;
}

static void generar_codigos_rec(CompresorHuffman *c, const NodoHuffman *n,
                                char ruta[SIMBOLOS], unsigned int profundidad)
{
    if (!n) return;
    if (n->simbolo >= 0) {
        if (profundidad == 0) ruta[profundidad++] = '0';
        ruta[profundidad] = '\0';
        strcpy(c->codigos[n->simbolo], ruta);
        return;
    }
    ruta[profundidad] = '0';
    generar_codigos_rec(c, n->izq, ruta, profundidad + 1);
    ruta[profundidad] = '1';
    generar_codigos_rec(c, n->der, ruta, profundidad + 1);
}

/* Enteros del encabezado almacenados en orden little-endian, portable. */
static int escribir_u16(FILE *f, uint16_t x)
{
    return fputc(x & 255, f) != EOF && fputc(x >> 8, f) != EOF;
}

static int escribir_u64(FILE *f, uint64_t x)
{
    for (int i = 0; i < 8; ++i, x >>= 8)
        if (fputc((unsigned char)(x & 255), f) == EOF) return 0;
    return 1;
}

static int leer_u16(FILE *f, uint16_t *x)
{
    int a = fgetc(f), b = fgetc(f);
    if (a == EOF || b == EOF) return 0;
    *x = (uint16_t)((unsigned)a | ((unsigned)b << 8));
    return 1;
}

static int leer_u64(FILE *f, uint64_t *x)
{
    *x = 0;
    for (int i = 0; i < 8; ++i) {
        int b = fgetc(f);
        if (b == EOF) return 0;
        *x |= (uint64_t)(unsigned char)b << (8 * i);
    }
    return 1;
}

/* Los códigos se empaquetan de 8 en 8; el último byte se rellena con ceros. */
static int escribir_bit(FILE *f, EstadoBits *e, int bit)
{
    e->byte = (unsigned char)((e->byte << 1) | (bit & 1));
    if (++e->bits == 8) {
        if (fputc(e->byte, f) == EOF) return 0;
        e->bits = 0;
        e->byte = 0;
    }
    return 1;
}

static int completar_byte(FILE *f, EstadoBits *e)
{
    if (!e->bits) return 1;
    return fputc((unsigned char)(e->byte << (8 - e->bits)), f) != EOF;
}

static int leer_bit(FILE *f, EstadoBits *e)
{
    if (e->bits == 0) {
        int x = fgetc(f);
        if (x == EOF) return -1;
        e->byte = (unsigned char)x;
        e->bits = 8;
    }
    int bit = (e->byte >> 7) & 1;
    e->byte <<= 1;
    --e->bits;
    return bit;
}

static void estadisticas(uint64_t original, long comprimido)
{
    printf("Original:   %" PRIu64 " bytes\n", original);
    printf("Comprimido: %ld bytes (incluye el árbol en forma de frecuencias)\n", comprimido);
    if (original)
        printf("Reducción:  %.2f%%\n", 100.0 * (1.0 - (double)comprimido / (double)original));
}

static int comprimir(const char *ruta_entrada, const char *ruta_salida)
{
    if (strcmp(ruta_entrada, ruta_salida) == 0) {
        fprintf(stderr, "Error: entrada y salida deben ser distintas.\n");
        return 1;
    }
    FILE *entrada = fopen(ruta_entrada, "rb");
    if (!entrada) { perror(ruta_entrada); return 1; }

    uint64_t frecuencias[SIMBOLOS] = {0}, total = 0;
    int b;
    while ((b = fgetc(entrada)) != EOF) {
        if (total == UINT64_MAX) {
            fprintf(stderr, "Error: archivo demasiado grande.\n");
            fclose(entrada);
            return 1;
        }
        ++frecuencias[b];
        ++total;
    }
    if (ferror(entrada) || fseek(entrada, 0, SEEK_SET) != 0) {
        perror("Lectura del archivo"); fclose(entrada); return 1;
    }

    CompresorHuffman c = {0};
    NodoHuffman *raiz = construir_arbol(&c, frecuencias);
    char ruta[SIMBOLOS] = {0};
    generar_codigos_rec(&c, raiz, ruta, 0);

    FILE *salida = fopen(ruta_salida, "wb");
    if (!salida) { perror(ruta_salida); fclose(entrada); return 1; }

    uint16_t distintos = 0;
    for (int s = 0; s < SIMBOLOS; ++s) if (frecuencias[s]) ++distintos;

    /* Formato: HUF1 + tamaño original (8 B) + N (2 B)
       + N parejas [símbolo (1 B), frecuencia (8 B)] + bits Huffman. */
    int ok = fwrite("HUF1", 1, 4, salida) == 4 &&
             escribir_u64(salida, total) && escribir_u16(salida, distintos);
    for (int s = 0; ok && s < SIMBOLOS; ++s)
        if (frecuencias[s])
            ok = fputc(s, salida) != EOF && escribir_u64(salida, frecuencias[s]);

    EstadoBits bits = {0};
    while (ok && (b = fgetc(entrada)) != EOF) {
        for (const char *p = c.codigos[b]; *p; ++p) {
            if (!escribir_bit(salida, &bits, *p == '1')) { ok = 0; break; }
        }
    }
    if (ferror(entrada)) ok = 0;
    if (ok) ok = completar_byte(salida, &bits);
    long tamano = ftell(salida);
    if (tamano < 0) ok = 0;
    if (fclose(salida) != 0) ok = 0;
    fclose(entrada);

    if (!ok) {
        fprintf(stderr, "Error: no se pudo completar la compresión.\n");
        remove(ruta_salida);
        return 1;
    }
    printf("Archivo comprimido: %s\n", ruta_salida);
    estadisticas(total, tamano);
    return 0;
}

static int descomprimir(const char *ruta_entrada, const char *ruta_salida)
{
    if (strcmp(ruta_entrada, ruta_salida) == 0) {
        fprintf(stderr, "Error: entrada y salida deben ser distintas.\n");
        return 1;
    }
    FILE *entrada = fopen(ruta_entrada, "rb");
    if (!entrada) { perror(ruta_entrada); return 1; }

    char firma[4];
    uint64_t total;
    uint16_t distintos;
    int ok = fread(firma, 1, 4, entrada) == 4 &&
             memcmp(firma, "HUF1", 4) == 0 &&
             leer_u64(entrada, &total) && leer_u16(entrada, &distintos) &&
             distintos <= SIMBOLOS;

    uint64_t frecuencias[SIMBOLOS] = {0}, suma = 0;
    for (unsigned i = 0; ok && i < distintos; ++i) {
        int s = fgetc(entrada);
        uint64_t frec;
        if (s == EOF || !leer_u64(entrada, &frec) || !frec ||
            frecuencias[s] != 0 || frec > total - suma) {
            ok = 0;
        } else {
            frecuencias[s] = frec;
            suma += frec;
        }
    }
    if (suma != total || (total > 0 && distintos == 0)) ok = 0;
    if (!ok) {
        fprintf(stderr, "Error: encabezado Huffman inválido o incompleto.\n");
        fclose(entrada);
        return 1;
    }

    CompresorHuffman c = {0};
    NodoHuffman *raiz = construir_arbol(&c, frecuencias);
    FILE *salida = fopen(ruta_salida, "wb");
    if (!salida) { perror(ruta_salida); fclose(entrada); return 1; }

    EstadoBits bits = {0};
    uint64_t obtenidas[SIMBOLOS] = {0};
    for (uint64_t i = 0; ok && i < total; ++i) {
        NodoHuffman *n = raiz;
        if (!n) { ok = 0; break; }
        if (n->simbolo >= 0) {
            /* Un solo símbolo: su código se fija en "0". */
            if (leer_bit(entrada, &bits) != 0) { ok = 0; break; }
        } else {
            while (n->simbolo < 0) {
                int bit = leer_bit(entrada, &bits);
                if (bit < 0) { ok = 0; break; }
                n = bit ? n->der : n->izq;
                if (!n) { ok = 0; break; }
            }
        }
        if (ok) {
            if (fputc(n->simbolo, salida) == EOF) ok = 0;
            else if (++obtenidas[n->simbolo] > frecuencias[n->simbolo]) ok = 0;
        }
    }
    for (int s = 0; ok && s < SIMBOLOS; ++s)
        if (obtenidas[s] != frecuencias[s]) ok = 0;

    /* Los bits finales de relleno deben ser cero y no debe sobrar información. */
    if (ok && bits.bits != 0 && bits.byte != 0) ok = 0;
    if (ok && fgetc(entrada) != EOF) ok = 0;
    if (fclose(salida) != 0) ok = 0;
    fclose(entrada);

    if (!ok) {
        fprintf(stderr, "Error: archivo comprimido truncado o corrupto.\n");
        remove(ruta_salida);
        return 1;
    }
    printf("Archivo descomprimido: %s\n", ruta_salida);
    printf("Recuperados: %" PRIu64 " bytes\n", total);
    return 0;
}

static void ayuda(const char *programa)
{
    printf("Uso:\n"
           "  %s c archivo_entrada archivo_comprimido\n"
           "  %s d archivo_comprimido archivo_recuperado\n"
           "  %s                 (ejemplo automático original)\n", programa, programa, programa);
}

int main(int argc, char *argv[])
{
    if (argc == 4) {
        if (strcmp(argv[1], "c") == 0) return comprimir(argv[2], argv[3]);
        if (strcmp(argv[1], "d") == 0) return descomprimir(argv[2], argv[3]);
    }
    if (argc == 1) {
        const char *ejemplo = "este es un texto de prueba para el algoritmo de huffman "
                              "clase de programacion 20 de octubre del 2026 10 a 12 am";
        FILE *f = fopen("origen.txt", "wb");
        if (!f) { perror("origen.txt"); return 1; }
        if (fwrite(ejemplo, 1, strlen(ejemplo), f) != strlen(ejemplo) || fclose(f) != 0) {
            fprintf(stderr, "No se pudo crear origen.txt.\n");
            return 1;
        }
        if (comprimir("origen.txt", "archivo.bin")) return 1;
        return descomprimir("archivo.bin", "recuperado.txt");
    }
    ayuda(argv[0]);
    return 1;
}
