#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAMANIO 10

typedef struct Nodo {
    char *clave;
    int valor;
    struct Nodo *siguiente;
} Nodo;

typedef struct {
    Nodo *posiciones[TAMANIO];
} HashTable;

/* Calcula la posicion a partir de los caracteres de la clave. */
static unsigned int funcion_hash(const char *clave) {
    unsigned int resultado = 0;

    for (const unsigned char *p = (const unsigned char *)clave; *p; ++p)
        resultado = resultado * 31u + *p;

    return resultado % TAMANIO;
}

/* Localiza una clave en su lista de colisiones. */
static Nodo *localizar(HashTable *tabla, const char *clave) {
    Nodo *nodo = tabla->posiciones[funcion_hash(clave)];

    for (; nodo != NULL; nodo = nodo->siguiente)
        if (strcmp(nodo->clave, clave) == 0)
            return nodo;

    return NULL;
}

/* Inserta una clave o actualiza su valor. Devuelve 0 si falta memoria. */
static int hash_insertar(HashTable *tabla, const char *clave, int valor) {
    Nodo *existente = localizar(tabla, clave);
    if (existente != NULL) {
        existente->valor = valor;
        return 1;
    }

    Nodo *nuevo = malloc(sizeof(*nuevo));
    if (nuevo == NULL)
        return 0;

    nuevo->clave = malloc(strlen(clave) + 1);
    if (nuevo->clave == NULL) {
        free(nuevo);
        return 0;
    }

    strcpy(nuevo->clave, clave);
    nuevo->valor = valor;

    unsigned int indice = funcion_hash(clave);
    nuevo->siguiente = tabla->posiciones[indice];
    tabla->posiciones[indice] = nuevo;
    return 1;
}

/* Devuelve el valor y comunica mediante encontrado si existe la clave. */
static int hash_buscar(HashTable *tabla, const char *clave, int *encontrado) {
    Nodo *nodo = localizar(tabla, clave);
    *encontrado = (nodo != NULL);
    return nodo != NULL ? nodo->valor : -1;
}

/* Elimina una clave sin perder el resto de la lista enlazada. */
static int hash_borrar(HashTable *tabla, const char *clave) {
    Nodo **enlace = &tabla->posiciones[funcion_hash(clave)];

    while (*enlace != NULL) {
        Nodo *actual = *enlace;
        if (strcmp(actual->clave, clave) == 0) {
            *enlace = actual->siguiente;
            free(actual->clave);
            free(actual);
            return 1;
        }
        enlace = &actual->siguiente;
    }
    return 0;
}

/* Libera todas las listas y deja la tabla vacia. */
static void hash_destruir(HashTable *tabla) {
    for (int i = 0; i < TAMANIO; ++i) {
        Nodo *actual = tabla->posiciones[i];
        while (actual != NULL) {
            Nodo *siguiente = actual->siguiente;
            free(actual->clave);
            free(actual);
            actual = siguiente;
        }
        tabla->posiciones[i] = NULL;
    }
}

int main(void) {
    HashTable tabla = {0};
    const char *nombres[] = {"Ernesto", "Braulio", "Catalina", "Alondra", "Freddy"};
    const int valores[] = {10, 15, 56, 18, 59};
    const int cantidad = (int)(sizeof(valores) / sizeof(valores[0]));

    for (int i = 0; i < cantidad; ++i) {
        if (!hash_insertar(&tabla, nombres[i], valores[i])) {
            fprintf(stderr, "Error: no se pudo reservar memoria.\n");
            hash_destruir(&tabla);
            return EXIT_FAILURE;
        }
    }

    /* Como en el programa original, la segunda insercion actualiza la clave. */
    if (!hash_insertar(&tabla, "Ernesto", 18)) {
        fprintf(stderr, "Error: no se pudo actualizar la clave.\n");
        hash_destruir(&tabla);
        return EXIT_FAILURE;
    }

    int encontrado;
    int valor = hash_buscar(&tabla, "Ernesto", &encontrado);
    if (encontrado)
        printf("Ernesto: %d\n", valor);

    if (hash_borrar(&tabla, "Braulio"))
        printf("Braulio eliminado correctamente.\n");

    hash_buscar(&tabla, "Braulio", &encontrado);
    printf("Braulio existe: %s\n", encontrado ? "si" : "no");

    hash_destruir(&tabla);
    return EXIT_SUCCESS;
}
