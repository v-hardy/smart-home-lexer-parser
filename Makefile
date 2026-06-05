# Detectar compilador disponible
CC := $(or $(shell command -v gcc 2>/dev/null), $(shell command -v clang 2>/dev/null))
CFLAGS := -Wall -std=c11

.PHONY: all test clean

all: lexer

lexer: main.c
	$(CC) $(CFLAGS) main.c -o lexer

test: main.c test_lexer.c
	$(CC) $(CFLAGS) -DTESTING test_lexer.c -o test_lexer
	./test_lexer

clean:
	rm -f lexer test_lexer
