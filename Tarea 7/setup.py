from setuptools import Extension, setup
from Cython.Build import cythonize

setup(
    name="huffman-cython",
    version="1.0",
    ext_modules=cythonize(
        [Extension("huffman_cython", ["huffman_cython.pyx"])],
        compiler_directives={"language_level": "3"},
    ),
)
