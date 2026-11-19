CC       := gcc
CFLAGS   := -O3 -fno-tree-vectorize -std=c11 -Wall -Wextra -Wpedantic -fPIC -msse2

.PHONY: all run test clean
all: libbordes.so

libbordes.so: bordes.c
	$(CC) $(CFLAGS) -shared -o $@ $<

run: libbordes.so
	python3 ejecutar.py

test: libbordes.so
	python3 -m unittest -v test_bordes.py

clean:
	rm -f libbordes.so
