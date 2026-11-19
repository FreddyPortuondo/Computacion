"""Pruebas reproducibles de correctitud, incluidos anchos que no son multiplos de 16."""
import ctypes
import random
import unittest
from ejecutar import cargar_biblioteca


def referencia_python(pixeles, ancho, alto, umbral):
    """Oraculo independiente de la implementacion en C."""
    resultado = bytearray(ancho * alto)
    for y in range(1, alto - 1):
        for x in range(1, ancho - 1):
            i = y * ancho + x
            gx = abs(pixeles[i + 1] - pixeles[i - 1])
            gy = abs(pixeles[i + ancho] - pixeles[i - ancho])
            resultado[i] = 255 if min(255, gx + gy) > umbral else 0
    return bytes(resultado)


class PruebasBordes(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = cargar_biblioteca()

    def test_equivalencia_sse2_escalar(self):
        rng = random.Random(20261119)
        for ancho, alto in ((3, 3), (16, 5), (17, 5), (18, 5),
                            (19, 9), (31, 8), (32, 31), (33, 33),
                            (64, 23), (127, 35)):
            n = ancho * alto
            vector = ctypes.c_uint8 * n
            pixeles = vector(*(rng.randrange(256) for _ in range(n)))
            escalar, sse2 = vector(), vector()
            for umbral in (0, 1, 63, 127, 254, 255):
                with self.subTest(ancho=ancho, alto=alto, umbral=umbral):
                    a = self.lib.detectar_bordes_escalar(pixeles, escalar, ancho, alto, umbral)
                    b = self.lib.detectar_bordes_sse2(pixeles, sse2, ancho, alto, umbral)
                    self.assertEqual(a, 0)
                    self.assertEqual(b, 0)
                    self.assertEqual(bytes(escalar), bytes(sse2))
                    self.assertEqual(bytes(sse2),
                                     referencia_python(pixeles, ancho, alto, umbral))
                    self.assertTrue(all(sse2[x] == 0 for x in range(ancho)))
                    self.assertTrue(all(sse2[(alto - 1) * ancho + x] == 0 for x in range(ancho)))
                    self.assertTrue(all(sse2[y * ancho] == 0 for y in range(alto)))
                    self.assertTrue(all(sse2[y * ancho + ancho - 1] == 0 for y in range(alto)))

    def test_argumentos_invalidos(self):
        vector = ctypes.c_uint8 * 16
        a, b = vector(), vector()
        self.assertEqual(self.lib.detectar_bordes_sse2(a, b, 2, 8, 50), -1)
        self.assertEqual(self.lib.detectar_bordes_sse2(a, b, 8, 2, 50), -1)
        self.assertEqual(self.lib.detectar_bordes_sse2(None, b, 4, 4, 50), -1)
        self.assertEqual(self.lib.detectar_bordes_sse2(a, None, 4, 4, 50), -1)


if __name__ == "__main__":
    unittest.main()
