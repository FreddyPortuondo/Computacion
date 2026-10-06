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
} Cola;

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

static int crear_cola(Cola *cola) {
    cola->Inicial = crear_nodo();
    cola->Final = crear_nodo();

    if (cola->Inicial == NULL || cola->Final == NULL) {
        free(cola->Inicial);
        free(cola->Final);
        cola->Inicial = NULL;
        cola->Final = NULL;
        cola->len = 0;
        return 0;
    }

    cola->Inicial->next = cola->Final;
    cola->Final->prev = cola->Inicial;
    cola->len = 0;
    return 1;
}

/* FIFO: el nuevo elemento entra por el final. */
static int encolar(Cola *cola, const char *nombre,
                   const char *direccion, int edad) {
    Registro *nuevo = crear_nodo();
    if (nuevo == NULL) {
        return 0;
    }

    snprintf(nuevo->nombre, sizeof nuevo->nombre, "%s", nombre);
    snprintf(nuevo->direccion, sizeof nuevo->direccion, "%s", direccion);
    nuevo->edad = edad;

    nuevo->prev = cola->Final->prev;
    nuevo->next = cola->Final;
    cola->Final->prev->next = nuevo;
    cola->Final->prev = nuevo;
    cola->len++;
    return 1;
}

/* FIFO: sale el elemento mas antiguo, ubicado al inicio. */
static int desencolar(Cola *cola, Registro *salida) {
    Registro *primero;

    if (cola->len == 0) {
        return 0;
    }

    primero = cola->Inicial->next;

    if (salida != NULL) {
        snprintf(salida->nombre, sizeof salida->nombre, "%s", primero->nombre);
        snprintf(salida->direccion, sizeof salida->direccion, "%s", primero->direccion);
        salida->edad = primero->edad;
        salida->next = NULL;
        salida->prev = NULL;
    }

    cola->Inicial->next = primero->next;
    primero->next->prev = cola->Inicial;
    free(primero);
    cola->len--;
    return 1;
}

static void mostrar_cola(const Cola *cola) {
    const Registro *iter;
    int posicion = 1;

    if (cola->len == 0) {
        printf("\nLa cola esta vacia.\n");
        return;
    }

    printf("\n--- COLA FIFO: primero -> ultimo ---\n");
    for (iter = cola->Inicial->next; iter != cola->Final; iter = iter->next) {
        printf("[%d] Nombre: %s | Direccion: %s | Edad: %d\n",
               posicion++, iter->nombre, iter->direccion, iter->edad);
    }
    printf("Elementos: %d\n", cola->len);
}

static void destruir_cola(Cola *cola) {
    Registro *actual = cola->Inicial;

    while (actual != NULL) {
        Registro *siguiente = actual->next;
        free(actual);
        actual = siguiente;
    }

    cola->Inicial = NULL;
    cola->Final = NULL;
    cola->len = 0;
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
    Cola cola;
    int opcion;

    if (!crear_cola(&cola)) {
        fprintf(stderr, "Error: no se pudo reservar memoria para la cola.\n");
        return EXIT_FAILURE;
    }

    do {
        printf("\n===== COLA FIFO =====\n");
        printf("1. Encolar registro\n");
        printf("2. Desencolar registro\n");
        printf("3. Mostrar cola\n");
        printf("4. Mostrar cantidad de elementos\n");
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

                if (encolar(&cola, nombre, direccion, edad)) {
                    printf("Registro encolado correctamente.\n");
                } else {
                    printf("No se pudo encolar: memoria insuficiente.\n");
                }
                break;
            }

            case 2: {
                Registro eliminado;
                if (desencolar(&cola, &eliminado)) {
                    printf("Salio primero: %s, %s, %d anios.\n",
                           eliminado.nombre, eliminado.direccion, eliminado.edad);
                } else {
                    printf("No se puede desencolar: la cola esta vacia.\n");
                }
                break;
            }

            case 3:
                mostrar_cola(&cola);
                break;

            case 4:
                printf("Cantidad de elementos: %d\n", cola.len);
                break;

            case 0:
                break;

            default:
                printf("Opcion no valida.\n");
        }
    } while (opcion != 0);

    destruir_cola(&cola);
    return EXIT_SUCCESS;
}
