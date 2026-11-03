"""Interfaz para ejecutar el módulo Huffman compilado en Cython."""
import sys
from huffman_cython import CompresorHuffman


def main():
    if len(sys.argv) == 4 and sys.argv[1] == "c":
        CompresorHuffman(sys.argv[2]).comprimir(sys.argv[3])
    elif len(sys.argv) == 4 and sys.argv[1] == "d":
        CompresorHuffman().descomprimir(sys.argv[2], sys.argv[3])
    elif len(sys.argv) == 1:
        ejemplo = (
            "este es un texto de prueba para el algoritmo de huffman "
            "clase de programacion 20 de octubre del 2026 10 a 12 am"
        )
        with open("origen.txt", "w", encoding="utf-8") as f:
            f.write(ejemplo)
        c = CompresorHuffman("origen.txt")
        c.comprimir("archivo.bin")
        c.descomprimir("archivo.bin", "recuperado.txt")
    else:
        print("Uso:")
        print("  python3 huffman_cli.py c entrada.txt comprimido.huf")
        print("  python3 huffman_cli.py d comprimido.huf recuperado.txt")
        print("  python3 huffman_cli.py     # demostración automática")
        return 1
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError) as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
