# =========================
# Configuración general
# =========================

CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -g

TARGET  := dsl
SRC     := main.c lexer.c parser.c
OBJ     := $(SRC:.c=.o)

# =========================
# Reglas principales
# =========================

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# =========================
# Compilación de objetos
# =========================

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# =========================
# Limpieza
# =========================

clean:
	rm -f $(OBJ) $(TARGET)

# =========================
# Ejecutar (Linux/macOS)
# =========================

run: $(TARGET)
	./$(TARGET) test.dsl

# =========================
# Ejecutar (Windows)
# =========================

run-win: $(TARGET)
	$(TARGET).exe test.dsl

# =========================
# Debug lexer
# =========================

tokens: $(TARGET)
	./$(TARGET) test.dsl --tokens

# =========================
# PHONY
# =========================

.PHONY: all clean run run-win tokens