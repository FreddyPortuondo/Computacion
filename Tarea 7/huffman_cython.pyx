# cython: language_level=3, boundscheck=False, wraparound=False
"""Compresor Huffman en Cython.

Formato HUF1 compatible con huffman.c:
  firma (4 bytes), longitud original (uint64 LE), cantidad de símbolos
  (uint16 LE), pares (símbolo:uint8, frecuencia:uint64 LE) y bits Huffman.
"""

import heapq
import os
import struct
from libc.stdint cimport uint64_t


cdef class NodoHuffman:
    cdef public int char
    cdef public uint64_t freq
    cdef public int menor_simbolo
    cdef public object left, right

    def __init__(self, int char, uint64_t freq, left=None, right=None):
        self.char = char
        self.freq = freq
        self.left = left
        self.right = right
        if char >= 0:
            self.menor_simbolo = char
        else:
            self.menor_simbolo = min(left.menor_simbolo, right.menor_simbolo)


cdef class CompresorHuffman:
    cdef object ruta_archivo
    cdef list heap, codigos
    cdef dict inverso_codigos

    def __init__(self, ruta_archivo=None):
        self.ruta_archivo = ruta_archivo
        self.heap = []
        self.codigos = [b""] * 256
        self.inverso_codigos = {}

    cdef list _calcular_frecuencias(self, bytes datos):
        cdef uint64_t frecuencias[256]
        cdef const unsigned char[:] vista = datos
        cdef Py_ssize_t i
        for i in range(256):
            frecuencias[i] = 0
        for i in range(len(vista)):
            frecuencias[vista[i]] += 1
        return [frecuencias[i] for i in range(256)]

    cdef object _construir_arbol(self, list frecuencias):
        cdef int simbolo
        cdef NodoHuffman nodo, nodo1, nodo2, combinado
        self.heap = []
        for simbolo in range(256):
            if frecuencias[simbolo]:
                nodo = NodoHuffman(simbolo, frecuencias[simbolo])
                heapq.heappush(self.heap,
                               (nodo.freq, nodo.menor_simbolo, nodo))

        while len(self.heap) > 1:
            nodo1 = <NodoHuffman>heapq.heappop(self.heap)[2]
            nodo2 = <NodoHuffman>heapq.heappop(self.heap)[2]
            combinado = NodoHuffman(-1, nodo1.freq + nodo2.freq,
                                    nodo1, nodo2)
            heapq.heappush(self.heap,
                           (combinado.freq, combinado.menor_simbolo, combinado))
        return self.heap[0][2] if self.heap else None

    cdef void _generar_codigos_rec(self, NodoHuffman nodo,
                                  bytes codigo_actual):
        if nodo.char >= 0:
            # Caso especial: archivo compuesto por un único símbolo.
            if not codigo_actual:
                codigo_actual = b"0"
            self.codigos[nodo.char] = codigo_actual
            self.inverso_codigos[codigo_actual] = nodo.char
            return
        self._generar_codigos_rec(<NodoHuffman>nodo.left,
                                  codigo_actual + b"0")
        self._generar_codigos_rec(<NodoHuffman>nodo.right,
                                  codigo_actual + b"1")

    def comprimir(self, ruta_salida):
        """Comprime el archivo dado al constructor; retorna (códigos, raíz)."""
        cdef bytes datos, codigo
        cdef const unsigned char[:] vista
        cdef const unsigned char[:] codigo_vista
        cdef list frecuencias
        cdef object raiz
        cdef bytearray salida_bits = bytearray()
        cdef Py_ssize_t i, j
        cdef unsigned int byte_actual = 0
        cdef unsigned int usados = 0
        cdef int simbolo, distintos = 0

        if self.ruta_archivo is None:
            raise ValueError("Indica el archivo de entrada al crear el compresor")
        if os.path.realpath(self.ruta_archivo) == os.path.realpath(ruta_salida):
            raise ValueError("Los archivos de entrada y salida deben ser distintos")

        with open(self.ruta_archivo, "rb") as f:
            datos = f.read()
        frecuencias = self._calcular_frecuencias(datos)
        raiz = self._construir_arbol(frecuencias)
        self.codigos = [b""] * 256
        self.inverso_codigos = {}
        if raiz is not None:
            self._generar_codigos_rec(<NodoHuffman>raiz, b"")

        for simbolo in range(256):
            if frecuencias[simbolo]:
                distintos += 1

        with open(ruta_salida, "wb") as f:
            f.write(struct.pack("<4sQH", b"HUF1", len(datos), distintos))
            for simbolo in range(256):
                if frecuencias[simbolo]:
                    f.write(struct.pack("<BQ", simbolo, frecuencias[simbolo]))

            vista = datos
            for i in range(len(vista)):
                codigo = <bytes>self.codigos[vista[i]]
                codigo_vista = codigo
                for j in range(len(codigo_vista)):
                    byte_actual = (byte_actual << 1) | (codigo_vista[j] == 49)
                    usados += 1
                    if usados == 8:
                        salida_bits.append(byte_actual)
                        byte_actual = 0
                        usados = 0
                        if len(salida_bits) >= 65536:
                            f.write(salida_bits)
                            salida_bits.clear()
            if usados:
                salida_bits.append(byte_actual << (8 - usados))
            f.write(salida_bits)

        cdef Py_ssize_t tamano = os.path.getsize(ruta_salida)
        print(f"Archivo comprimido: {ruta_salida}")
        print(f"Original:   {len(datos)} bytes")
        print(f"Comprimido: {tamano} bytes (incluye frecuencias)")
        if datos:
            print(f"Reducción:  {100 * (1 - tamano / len(datos)):.2f}%")
        return self.codigos, raiz

    def descomprimir(self, ruta_comprimido, ruta_salida):
        """Recupera el archivo desde HUF1 sin necesitar el árbol en memoria."""
        cdef bytes encabezado, pareja, contenido
        cdef const unsigned char[:] vista
        cdef int simbolo, distintos, bit
        cdef uint64_t total, frecuencia
        cdef list frecuencias = [0] * 256
        cdef list verificadas = [0] * 256
        cdef object raiz
        cdef NodoHuffman actual
        cdef bytearray recuperado = bytearray()
        cdef Py_ssize_t pos = 0
        cdef Py_ssize_t longitud_bits, i, j
        cdef uint64_t k

        if os.path.realpath(ruta_comprimido) == os.path.realpath(ruta_salida):
            raise ValueError("Los archivos de entrada y salida deben ser distintos")

        with open(ruta_comprimido, "rb") as f:
            encabezado = f.read(14)
            if len(encabezado) != 14:
                raise ValueError("Archivo HUF1 incompleto")
            firma, total, distintos = struct.unpack("<4sQH", encabezado)
            if firma != b"HUF1" or distintos > 256:
                raise ValueError("Firma o encabezado Huffman inválido")
            for i in range(distintos):
                pareja = f.read(9)
                if len(pareja) != 9:
                    raise ValueError("Tabla de frecuencias incompleta")
                simbolo, frecuencia = struct.unpack("<BQ", pareja)
                if frecuencia == 0 or frecuencias[simbolo] != 0:
                    raise ValueError("Tabla de frecuencias inválida")
                frecuencias[simbolo] = frecuencia
            if sum(frecuencias) != total:
                raise ValueError("Las frecuencias no coinciden con el tamaño")
            contenido = f.read()

        raiz = self._construir_arbol(frecuencias)
        vista = contenido
        longitud_bits = len(vista) * 8
        if total and raiz is None:
            raise ValueError("Archivo HUF1 sin árbol de decodificación")

        for k in range(total):
            actual = <NodoHuffman>raiz
            if actual.char >= 0:
                # El código para un solo carácter es siempre 0.
                if pos >= longitud_bits:
                    raise ValueError("Datos Huffman truncados")
                bit = (vista[pos // 8] >> (7 - pos % 8)) & 1
                pos += 1
                if bit != 0:
                    raise ValueError("Código Huffman inválido")
            else:
                while actual.char < 0:
                    if pos >= longitud_bits:
                        raise ValueError("Datos Huffman truncados")
                    bit = (vista[pos // 8] >> (7 - pos % 8)) & 1
                    pos += 1
                    if bit:
                        actual = <NodoHuffman>actual.right
                    else:
                        actual = <NodoHuffman>actual.left
            recuperado.append(actual.char)
            verificadas[actual.char] += 1
            if verificadas[actual.char] > frecuencias[actual.char]:
                raise ValueError("Frecuencia inesperada: archivo dañado")

        if verificadas != frecuencias:
            raise ValueError("Frecuencias no coinciden: archivo dañado")
        if longitud_bits - pos > 7:
            raise ValueError("Archivo contiene bytes adicionales")
        for j in range(pos, longitud_bits):
            if (vista[j // 8] >> (7 - j % 8)) & 1:
                raise ValueError("Bits de relleno inválidos")

        with open(ruta_salida, "wb") as f:
            f.write(recuperado)
        print(f"Archivo descomprimido: {ruta_salida}")
        print(f"Recuperados: {len(recuperado)} bytes")
        return bytes(recuperado)
