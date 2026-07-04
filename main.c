#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"


int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s [tokens|parse] archivo.smart\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *mode = argv[1];
    const char *file = argv[2];

    if (!abrirFuente(file))
    {
        fprintf(stderr, "No se pudo abrir el archivo: %s\n", file);
        return EXIT_FAILURE;
    }

    if (strcmp(mode, "tokens") == 0)
    {
        volcarTokens();
    }
    else if (strcmp(mode, "parse") == 0)
    {
        parsePrograma();  
    }
    else
    {
        fprintf(stderr, "Modo desconocido: %s\n", mode);
        cerrarFuente();
        return EXIT_FAILURE;
    }

    cerrarFuente();
    return EXIT_SUCCESS;
}