#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM_NOMBRE 50
#define TAM_DIRECCION 50

typedef struct registro {
    char nombre[TAM_NOMBRE];
    char direccion[TAM_DIRECCION];
    int edad;
    struct registro *next;
    struct registro *prev;
} Registro;

typedef struct {
    Registro *Inicial;   /* Nodo centinela inicial */
    Registro *Final;     /* Nodo centinela final   */
    int len;
} Pila;

static Registro *crear_nodo(void) {
    Registro *nodo = malloc(sizeof *nodo);
    if (nodo != NULL) {
        nodo->nombre[0] = '\0';
        nodo->direccion[0] = '\0';
        nodo->edad = 0;
        nodo->next = NULL;
        nodo->prev = NULL;
    }
    return nodo;
}

static int crear_pila(Pila *pila) {
    pila->Inicial = crear_nodo();
    pila->Final = crear_nodo();

    if (pila->Inicial == NULL || pila->Final == NULL) {
        free(pila->Inicial);
        free(pila->Final);
        pila->Inicial = NULL;
        pila->Final = NULL;
        pila->len = 0;
        return 0;
    }

    pila->Inicial->next = pila->Final;
    pila->Final->prev = pila->Inicial;
    pila->len = 0;
    return 1;
}

/* LIFO: PUSH inserta en el tope, justo antes del nodo Final. */
static int push(Pila *pila, const char *nombre,
                const char *direccion, int edad) {
    Registro *nuevo = crear_nodo();
    if (nuevo == NULL) {
        return 0;
    }

    snprintf(nuevo->nombre, sizeof nuevo->nombre, "%s", nombre);
    snprintf(nuevo->direccion, sizeof nuevo->direccion, "%s", direccion);
    nuevo->edad = edad;

    nuevo->prev = pila->Final->prev;
    nuevo->next = pila->Final;
    pila->Final->prev->next = nuevo;
    pila->Final->prev = nuevo;
    pila->len++;
    return 1;
}

/* LIFO: POP elimina el ultimo elemento insertado. */
static int pop(Pila *pila, Registro *salida) {
    Registro *tope;

    if (pila->len == 0) {
        return 0;
    }

    tope = pila->Final->prev;

    if (salida != NULL) {
        snprintf(salida->nombre, sizeof salida->nombre, "%s", tope->nombre);
        snprintf(salida->direccion, sizeof salida->direccion, "%s", tope->direccion);
        salida->edad = tope->edad;
        salida->next = NULL;
        salida->prev = NULL;
    }

    tope->prev->next = pila->Final;
    pila->Final->prev = tope->prev;
    free(tope);
    pila->len--;
    return 1;
}

static int ver_tope(const Pila *pila, Registro *salida) {
    const Registro *tope;

    if (pila->len == 0) {
        return 0;
    }

    tope = pila->Final->prev;
    if (salida != NULL) {
        snprintf(salida->nombre, sizeof salida->nombre, "%s", tope->nombre);
        snprintf(salida->direccion, sizeof salida->direccion, "%s", tope->direccion);
        salida->edad = tope->edad;
        salida->next = NULL;
        salida->prev = NULL;
    }
    return 1;
}

static void mostrar_pila(const Pila *pila) {
    const Registro *iter;
    int posicion = 1;

    if (pila->len == 0) {
        printf("\nLa pila esta vacia.\n");
        return;
    }

    printf("\n--- PILA LIFO: tope -> fondo ---\n");
    for (iter = pila->Final->prev; iter != pila->Inicial; iter = iter->prev) {
        printf("[%d] Nombre: %s | Direccion: %s | Edad: %d\n",
               posicion++, iter->nombre, iter->direccion, iter->edad);
    }
    printf("Elementos: %d\n", pila->len);
}

static void destruir_pila(Pila *pila) {
    Registro *actual = pila->Inicial;

    while (actual != NULL) {
        Registro *siguiente = actual->next;
        free(actual);
        actual = siguiente;
    }

    pila->Inicial = NULL;
    pila->Final = NULL;
    pila->len = 0;
}

static void leer_linea(const char *mensaje, char *buffer, size_t capacidad) {
    size_t longitud;

    printf("%s", mensaje);
    if (fgets(buffer, (int)capacidad, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }

    longitud = strcspn(buffer, "\n");
    if (buffer[longitud] == '\n') {
        buffer[longitud] = '\0';
    } else {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
            /* Descarta caracteres sobrantes de la entrada. */
        }
    }
}

static int leer_entero(const char *mensaje) {
    char linea[64];
    char *fin;
    long valor;

    for (;;) {
        leer_linea(mensaje, linea, sizeof linea);
        valor = strtol(linea, &fin, 10);
        if (fin != linea && *fin == '\0') {
            return (int)valor;
        }
        printf("Entrada invalida. Introduzca un numero entero.\n");
    }
}

int main(void) {
    Pila pila;
    int opcion;

    if (!crear_pila(&pila)) {
        fprintf(stderr, "Error: no se pudo reservar memoria para la pila.\n");
        return EXIT_FAILURE;
    }

    do {
        printf("\n===== PILA LIFO =====\n");
        printf("1. Push (apilar registro)\n");
        printf("2. Pop (desapilar registro)\n");
        printf("3. Ver tope\n");
        printf("4. Mostrar pila\n");
        printf("5. Mostrar cantidad de elementos\n");
        printf("0. Salir\n");
        opcion = leer_entero("Seleccione una opcion: ");

        switch (opcion) {
            case 1: {
                char nombre[TAM_NOMBRE];
                char direccion[TAM_DIRECCION];
                int edad;

                leer_linea("Nombre: ", nombre, sizeof nombre);
                leer_linea("Direccion: ", direccion, sizeof direccion);
                edad = leer_entero("Edad: ");

                if (push(&pila, nombre, direccion, edad)) {
                    printf("Registro apilado correctamente.\n");
                } else {
                    printf("No se pudo apilar: memoria insuficiente.\n");
                }
                break;
            }

            case 2: {
                Registro eliminado;
                if (pop(&pila, &eliminado)) {
                    printf("Salio del tope: %s, %s, %d anios.\n",
                           eliminado.nombre, eliminado.direccion, eliminado.edad);
                } else {
                    printf("No se puede desapilar: la pila esta vacia.\n");
                }
                break;
            }

            case 3: {
                Registro tope;
                if (ver_tope(&pila, &tope)) {
                    printf("Tope: %s, %s, %d anios.\n",
                           tope.nombre, tope.direccion, tope.edad);
                } else {
                    printf("La pila esta vacia.\n");
                }
                break;
            }

            case 4:
                mostrar_pila(&pila);
                break;

            case 5:
                printf("Cantidad de elementos: %d\n", pila.len);
                break;

            case 0:
                break;

            default:
                printf("Opcion no valida.\n");
        }
    } while (opcion != 0);

    destruir_pila(&pila);
    return EXIT_SUCCESS;
}
