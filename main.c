#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "interpreter.h"
#include "estado.h"
#include "html.h"


/*========================================
    Utilidades
========================================*/

static bool tieneExtensionSmart(const char *nombre)
{
    const char *ext = strrchr(nombre, '.');

    if (ext == NULL)
        return false;

    return strcmp(ext, ".smart") == 0;
}


/*========================================
    Modo interactivo
========================================*/

static void modoInteractivo(void)
{
    char linea[4096];

    printf("=========================================\n");
    printf(" Smart Home DSL - Modo Interactivo\n");
    printf(" (Tokenizador)\n");
    printf(" Escriba 'exit' para salir.\n");
    printf("=========================================\n");

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

/*========================================
    Help
========================================*/

static void mostrarAyuda(const char *programa)
{
    printf("\n");
    printf("Smart Home DSL\n");
    printf("---------------------------------------------\n");
    printf("Uso:\n\n");

    printf("  %s\n", programa);
    printf("      Inicia el modo interactivo (tokenizador).\n\n");

    printf("  %s tokens archivo.smart\n", programa);
    printf("      Ejecuta el analizador léxico.\n\n");

    printf("  %s parse archivo.smart\n", programa);
    printf("      Ejecuta el analizador sintáctico.\n\n");

    printf("  %s ast archivo.smart\n", programa);
    printf("      Construye e imprime el AST.\n\n");

    printf("  %s html archivo.smart\n", programa);
    printf("      Interpreta el programa y genera salida.html.\n\n");

    printf("  %s run [archivo.smart]\n", programa);
    printf("      Ejecuta el proceso completo.\n");
    printf("      Si no se especifica archivo utiliza programa1.smart.\n\n");

    printf("  %s help\n", programa);
    printf("  %s -h\n", programa);
    printf("  %s --help\n", programa);
    printf("      Muestra esta ayuda.\n\n");

    printf("Restricciones:\n");
    printf("  - Los archivos deben tener extensión .smart\n");
    printf("  - El archivo indicado debe existir.\n");
}


/*========================================
    Main
========================================*/

int main(int argc, char *argv[])
{
    /*------------------------------------
      Sin argumentos -> modo interactivo
    ------------------------------------*/

    if (argc == 1)
    {
        modoInteractivo();
        return EXIT_SUCCESS;
    }

    const char *modo = argv[1];
    const char *archivo = "programa1.smart";

    /*
        run admite archivo opcional.

        ./smart run
        ./smart run programa2.smart
    */

    if (strcmp(modo, "run") == 0)
    {
        if (argc >= 3)
            archivo = argv[2];
    }
    else
    {
        if (argc == 2 &&
            (strcmp(argv[1], "help") == 0 ||
            strcmp(argv[1], "-h") == 0 ||
            strcmp(argv[1], "--help") == 0))
        {
            mostrarAyuda(argv[0]);
            return EXIT_SUCCESS;
        }
        
        if (argc != 3)
        {
            fprintf(stderr,
                "\nUso:\n\n"
                "  %s tokens archivo.smart\n"
                "  %s parse  archivo.smart\n"
                "  %s ast    archivo.smart\n"
                "  %s html   archivo.smart\n"
                "  %s run [archivo.smart]\n\n",
                argv[0],
                argv[0],
                argv[0],
                argv[0],
                argv[0]);

            printf("  %s help\n", argv[0]);
            printf("  %s -h\n", argv[0]);
            printf("  %s --help\n", argv[0]);
            printf("      Para mayor detalle de uso.\n\n");

            return EXIT_FAILURE;
        }

        archivo = argv[2];
    }

    if (!tieneExtensionSmart(archivo))
    {
        fprintf(stderr,
                "Error: '%s' no es un archivo .smart válido.\n",
                archivo);

        return EXIT_FAILURE;
    }

    if (!abrirFuente(archivo))
    {
        fprintf(stderr,
                "No se pudo abrir el archivo: %s\n",
                archivo);

        return EXIT_FAILURE;
    }

    /*------------------------------------
      TOKENS
    ------------------------------------*/

    if (strcmp(modo, "tokens") == 0)
    {
        volcarTokens();
    }

    /*------------------------------------
      PARSER
    ------------------------------------*/

    else if (strcmp(modo, "parse") == 0)
    {
        modoDebugParse = 1;

        NodoAST *raiz = parsePrograma();

        liberarAST(raiz);
    }

    /*------------------------------------
      AST
    ------------------------------------*/

    else if (strcmp(modo, "ast") == 0)
    {
        NodoAST *raiz = parsePrograma();

        imprimirAST(raiz);

        liberarAST(raiz);
    }

    /*------------------------------------
      HTML
    ------------------------------------*/

    else if (strcmp(modo, "html") == 0 ||
             strcmp(modo, "run") == 0)
    {
        NodoAST *raiz = parsePrograma();

        EstadoSistema estado;

        interpretarPrograma(raiz, &estado);

        generarHTML("salida.html", &estado);

        if (strcmp(modo, "run") == 0)
        {
#ifdef __linux__
            system("xdg-open salida.html");
#elif _WIN32
            system("start salida.html");
#elif __APPLE__
            system("open salida.html");
#endif
        }

        destruirEstado(&estado);
        liberarAST(raiz);
    }

    /*------------------------------------
      Error
    ------------------------------------*/

    else
    {
        fprintf(stderr,
                "Modo desconocido: %s\n",
                modo);

        cerrarFuente();

        return EXIT_FAILURE;
    }

    cerrarFuente();

    return EXIT_SUCCESS;
}