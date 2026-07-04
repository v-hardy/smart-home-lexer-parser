# =========================
# Configuración general
# =========================

CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -g

TARGET  := smart
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
	./$(TARGET) parse programa1.smart

# =========================
# Ejecutar (Windows)
# =========================

run-win: $(TARGET)
	$(TARGET).exe parse programa1.smart

# =========================
# Debug lexer
# =========================

tokens: $(TARGET)
	./$(TARGET) tokens programa1.smart

# =========================
# PHONY
# =========================

.PHONY: all clean run run-win tokens