#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

typedef struct {
    uint64_t high;  // Bits 127-64
    uint64_t low;   // Bits 63-0
} uint128_t;

uint128_t sumar_128(uint128_t a, uint128_t b)
{
    uint128_t resultado;

    // Sumar las partes bajas
    resultado.low = a.low + b.low;

    // Detectar acarreo
    uint64_t acarreo = (resultado.low < a.low);

    // Sumar las partes altas + acarreo
    resultado.high = a.high + b.high + acarreo;

    return resultado;
}

void imprimir_128(uint128_t numero)
{
    printf("0x%016" PRIx64 "%016" PRIx64 "\n",
           numero.high, numero.low);
}

int main(void)
{
    uint128_t numero1;
    uint128_t numero2;
    uint128_t resultado;

    // Numero 1 de 128 bits
    numero1.high = 0x123456789ABCDEF0ULL;
    numero1.low  = 0xFEDCBA9876543210ULL;

    // Numero 2 de 128 bits
    numero2.high = 0x1111111111111111ULL;
    numero2.low  = 0x2222222222222222ULL;

    // Realizar suma
    resultado = sumar_128(numero1, numero2);

    printf("===== NUMERO 1 =====\n");
    printf("Hexadecimal: ");
    imprimir_128(numero1);

    printf("Direccion de numero1:      %p\n", (void *)&numero1);
    printf("Direccion de numero1.high: %p\n", (void *)&numero1.high);
    printf("Direccion de numero1.low:  %p\n\n", (void *)&numero1.low);


    printf("===== NUMERO 2 =====\n");
    printf("Hexadecimal: ");
    imprimir_128(numero2);

    printf("Direccion de numero2:      %p\n", (void *)&numero2);
    printf("Direccion de numero2.high: %p\n", (void *)&numero2.high);
    printf("Direccion de numero2.low:  %p\n\n", (void *)&numero2.low);


    printf("===== RESULTADO =====\n");
    printf("Hexadecimal: ");
    imprimir_128(resultado);

    printf("Direccion de resultado:      %p\n", (void *)&resultado);
    printf("Direccion de resultado.high: %p\n", (void *)&resultado.high);
    printf("Direccion de resultado.low:  %p\n", (void *)&resultado.low);

    return 0;
}
