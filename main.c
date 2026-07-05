#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "config.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "interpreter.h"
#include "estado.h"
#include "html.h"

static void modoInteractivo(void)
{
    char linea[4096];

    printf("Modo interactivo. (TOKENIZADOR)\n");
    printf("Escriba 'exit' para salir.\n");

    while (1)
    {
        printf("> ");

        if (fgets(linea, sizeof(linea), stdin) == NULL)
            break;

        if (strcmp(linea, "exit\n") == 0)
            break;

        lexerInitDesdeString(linea);

        volcarTokens();
    }
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        modoInteractivo();
        return EXIT_SUCCESS;
    }

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
        modoDebugParse = 1;

        NodoAST *raiz = parsePrograma();
        liberarAST(raiz);
    }
    else if (strcmp(mode, "ast") == 0)
    {
        NodoAST *raiz = parsePrograma();
        imprimirAST(raiz);
        liberarAST(raiz);
    }
    else if (strcmp(mode, "html") == 0)
    {
        NodoAST *raiz = parsePrograma();

        EstadoSistema estado;

        interpretarPrograma(raiz, &estado);

        generarHTML("salida.html", &estado);

        #ifdef __linux__
            system("xdg-open salida.html");
        #elif _WIN32
            system("start salida.html");
        #elif __APPLE__
            system("open salida.html");
        #endif
        
        liberarAST(raiz);
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