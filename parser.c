#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "parser.h"
#include "lexer.h"

// <======================================= Prototipos internos =======================================>

static void parseInstruccion(void);
static bool iniciaInstruccion(TokenType t);

static void parseBloqueWhen(void);
static void parseBloqueEvery(void);
static void parseBloqueCondicional(void);
static void parseAsignacion(void);



/* Reporte de error sintactico con posicion */
void errorSintactico(const char *mensaje)
{
    fprintf(stderr,
        "Error sintáctico [línea %d, col %d]: %s\n",
        lookahead.linea,
        lookahead.columna,
        mensaje);

    exit(EXIT_FAILURE);
}

//Instrucciones ::= Instruccion+
void parsePrograma(void)
{
    do
    {
        parseInstruccion();
    }
    while (iniciaInstruccion(lookahead.tipo));

    match(TK_EOF);
}

void parseInstruccion(void) {
    switch (lookahead.tipo) {
        case TK_WHEN:
            parseBloqueWhen();
            break;

        case TK_EVERY:
            parseBloqueEvery();
            break;

        case TK_IF:
            parseBloqueCondicional();
            break;

        case TK_FOCO_ID:
        case TK_AIRE_ID:
        case TK_PERSIANA_ID:
        case TK_CERRADURA_ID:
        case TK_RELOJ_ID:
        case TK_ALTAVOZ_ID:
        case TK_ALARMA_ID:
            parseAsignacion();
            break;

        default:
            errorSintactico("Se esperaba una instruccion");
    }
}

bool iniciaInstruccion(TokenType t)
{
    switch(t)
    {
        case TK_WHEN:
        case TK_EVERY:
        case TK_IF:
        case TK_FOCO_ID:
        case TK_AIRE_ID:
        case TK_PERSIANA_ID:
        case TK_CERRADURA_ID:
        case TK_RELOJ_ID:
        case TK_ALTAVOZ_ID:
        case TK_ALARMA_ID:
            return true;

        default:
            return false;
    }
}



void parseBloqueWhen(void)        { /* TODO issue #9 */ }
void parseBloqueEvery(void)       { /* TODO issue #9 */ }
void parseBloqueCondicional(void) { /* TODO issue #9 */ }
void parseAsignacion(void)        { /* TODO issue #9 */ }
