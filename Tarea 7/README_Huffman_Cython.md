# Huffman en Cython

## Archivos necesarios
- `huffman_cython.pyx`: implementación de Huffman en Cython.
- `setup.py`: configuración para compilar el módulo.
- `huffman_cli.py`: programa de línea de comandos.
- `.gitignore`: evita subir a GitHub archivos compilados y temporales.

## Instalación de herramientas en Ubuntu/Debian

```bash
sudo apt update
sudo apt install python3 python3-dev python3-setuptools cython3 build-essential
```

Si ya tienes las herramientas instaladas, omite ese paso.

## Compilación

Abre una terminal en la carpeta que contenga los tres archivos de código:

```bash
cd ~/Freddy/Computacion/Tareas/"Tarea 6"
python3 setup.py build_ext --inplace
```

Se creará una biblioteca `.so` (su nombre incluye la versión de Python y la plataforma). La biblioteca es el módulo compilado, no es necesario copiarla a GitHub.

## Compresión

```bash
python3 huffman_cli.py c mexico_surgimiento_nacion.txt mexico_cython.huf
```

## Descompresión

```bash
python3 huffman_cli.py d mexico_cython.huf mexico_cython_recuperado.txt
```

## Demostración sin parámetros

```bash
python3 huffman_cli.py
```

Se generan `origen.txt`, `archivo.bin` y `recuperado.txt` como en el ejemplo de Python.

## Verificar que el resultado es idéntico al original

```bash
cmp mexico_surgimiento_nacion.txt mexico_cython_recuperado.txt
```

Si no aparece ninguna salida, ambos archivos son idénticos. También puedes ejecutar:

```bash
sha256sum mexico_surgimiento_nacion.txt mexico_cython_recuperado.txt
```

## Formato de archivo

El formato HUF1 es compatible con la implementación C anterior:

- Firma `HUF1` (4 bytes).
- Longitud original (uint64 little-endian, 8 bytes).
- Número de símbolos distintos (uint16 little-endian, 2 bytes).
- Frecuencias: para cada símbolo, 1 byte de símbolo y 8 bytes de frecuencia.
- Datos comprimidos con Huffman y hasta 7 bits de relleno final.

**Nota:** Huffman no siempre reduce el tamaño. En archivos pequeños o aleatorios, la cabecera puede hacer que el archivo comprimido ocupe más.

**Memoria:** esta implementación lee el archivo completo en memoria. Es apropiada para la tarea y los archivos de prueba; para archivos muy grandes sería preferible procesar bloques.
