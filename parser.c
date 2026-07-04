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

static void parseCondicion(void);
static void parseOrExpr(void);
static void parseAndExpr(void);
static void parseNotExpr(void);

static void parseExpresionLogica(void);
static void parseSensorExpr(void);
static void parseActuadorExpr(void);

static void parseOperComp(void);
static void parseOperIgualdad(void);

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
    siguienteToken(); 

    while (lookahead.tipo != TK_EOF)
    {
        parseInstruccion();
    }

    match(TK_EOF);

    printf("Programa sintácticamente correcto.\n");
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
            errorSintactico("Instrucción inválida");
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

void parseBloqueWhen(void)
{
    match(TK_WHEN);

    parseCondicion();

    match(TK_DO);

    while(iniciaInstruccion(lookahead.tipo))
        parseInstruccion();

    match(TK_END);
}

void parseBloqueEvery(void)
{
    match(TK_EVERY);

    match(TK_TIEMPO);

    match(TK_DO);

    while(iniciaInstruccion(lookahead.tipo))
        parseInstruccion();

    match(TK_END);
}

void parseBloqueCondicional(void)
{
    match(TK_IF);

    parseCondicion();

    match(TK_THEN);

    while(iniciaInstruccion(lookahead.tipo))
        parseInstruccion();

    if(lookahead.tipo == TK_ELSE)
    {
        match(TK_ELSE);

        while(iniciaInstruccion(lookahead.tipo))
            parseInstruccion();
    }

    match(TK_END);
}

void parseCondicion(void)
{
    parseOrExpr();
}

void parseOrExpr(void)
{
    parseAndExpr();

    while(lookahead.tipo == TK_OR)
    {
        match(TK_OR);
        parseAndExpr();
    }
}

void parseAndExpr(void)
{
    parseNotExpr();

    while(lookahead.tipo == TK_AND)
    {
        match(TK_AND);
        parseNotExpr();
    }
}

void parseNotExpr(void)
{
    if(lookahead.tipo == TK_NOT)
    {
        match(TK_NOT);
        parseNotExpr();
    }
    else if(lookahead.tipo == TK_PAR_IZQ)
    {
        match(TK_PAR_IZQ);
        parseCondicion();
        match(TK_PAR_DER);
    }
    else
    {
        parseExpresionLogica();
    }
}

void parseExpresionLogica(void)
{
    switch(lookahead.tipo)
    {
        case TK_SENSOR_TEMP:
        case TK_SENSOR_HUMEDAD:
        case TK_SENSOR_LUZ:
        case TK_SENSOR_MOVIMIENTO:
        case TK_SENSOR_HUMO:
            parseSensorExpr();
            break;

        case TK_FOCO_ID:
        case TK_AIRE_ID:
        case TK_PERSIANA_ID:
        case TK_CERRADURA_ID:
        case TK_RELOJ_ID:
        case TK_ALTAVOZ_ID:
        case TK_ALARMA_ID:
            parseActuadorExpr();
            break;

        default:
            errorSintactico("Se esperaba una expresión lógica");
    }
}

void parseSensorExpr(void)
{
    switch(lookahead.tipo)
    {
        case TK_SENSOR_TEMP:
            match(TK_SENSOR_TEMP);
            parseOperComp();
            match(TK_TEMP);
            break;

        case TK_SENSOR_HUMEDAD:
            match(TK_SENSOR_HUMEDAD);
            parseOperComp();
            match(TK_PORCENTAJE);
            break;

        case TK_SENSOR_LUZ:
            match(TK_SENSOR_LUZ);
            parseOperComp();
            match(TK_LUX);
            break;

        case TK_SENSOR_MOVIMIENTO:
            match(TK_SENSOR_MOVIMIENTO);
            parseOperIgualdad();
            match(TK_BOOL_SENSOR);
            break;

        case TK_SENSOR_HUMO:
            match(TK_SENSOR_HUMO);
            parseOperIgualdad();
            match(TK_BOOL_SENSOR);
            break;

        default:
            errorSintactico("Sensor inválido");
    }
}

void parseActuadorExpr(void)
{
    switch (lookahead.tipo)
    {
        case TK_FOCO_ID:
            match(TK_FOCO_ID);
            match(TK_DELIMITADOR);

            switch (lookahead.tipo)
            {
                case TK_ATRIB_ESTADO:
                    match(TK_ATRIB_ESTADO);
                    parseOperIgualdad();
                    match(TK_BOOL_ACTUADOR);
                    break;

                case TK_ATRIB_BRILLO:
                    match(TK_ATRIB_BRILLO);
                    parseOperComp();
                    match(TK_PORCENTAJE);
                    break;

                case TK_ATRIB_COLOR:
                    match(TK_ATRIB_COLOR);
                    parseOperIgualdad();
                    match(TK_COLOR);
                    break;

                default:
                    errorSintactico("Atributo inválido para FOCO");
            }
            break;

        case TK_AIRE_ID:
            match(TK_AIRE_ID);
            match(TK_DELIMITADOR);

            switch (lookahead.tipo)
            {
                case TK_ATRIB_ESTADO:
                    match(TK_ATRIB_ESTADO);
                    parseOperIgualdad();
                    match(TK_BOOL_ACTUADOR);
                    break;

                case TK_ATRIB_TEMP_O:
                    match(TK_ATRIB_TEMP_O);
                    parseOperComp();
                    match(TK_TEMP);
                    break;

                case TK_ATRIB_TEMP_A:
                    match(TK_ATRIB_TEMP_A);
                    parseOperComp();
                    match(TK_TEMP);
                    break;

                case TK_ATRIB_MODO:
                    match(TK_ATRIB_MODO);
                    parseOperIgualdad();
                    match(TK_MODO);
                    break;

                default:
                    errorSintactico("Atributo inválido para AIRE");
            }
            break;

        case TK_PERSIANA_ID:
            match(TK_PERSIANA_ID);
            match(TK_DELIMITADOR);

            switch (lookahead.tipo)
            {
                case TK_ATRIB_POSICION:
                    match(TK_ATRIB_POSICION);
                    parseOperComp();
                    match(TK_PORCENTAJE);
                    break;

                default:
                    errorSintactico("Atributo inválido para PERSIANA");
            }
            break;

        case TK_CERRADURA_ID:
            match(TK_CERRADURA_ID);
            match(TK_DELIMITADOR);

            switch (lookahead.tipo)
            {
                case TK_ATRIB_ESTADO:
                    match(TK_ATRIB_ESTADO);
                    parseOperIgualdad();
                    match(TK_BOOL_ACTUADOR);
                    break;

                default:
                    errorSintactico("Atributo inválido para CERRADURA");
            }
            break;

        case TK_RELOJ_ID:
            match(TK_RELOJ_ID);
            match(TK_DELIMITADOR);

            switch (lookahead.tipo)
            {
                case TK_ATRIB_HORA:
                    match(TK_ATRIB_HORA);
                    parseOperComp();
                    match(TK_HORA);
                    break;

                case TK_ATRIB_FECHA:
                    match(TK_ATRIB_FECHA);
                    parseOperIgualdad();
                    match(TK_FECHA);
                    break;

                default:
                    errorSintactico("Atributo inválido para RELOJ");
            }
            break;

        case TK_ALTAVOZ_ID:
            match(TK_ALTAVOZ_ID);
            match(TK_DELIMITADOR);

            switch (lookahead.tipo)
            {
                case TK_ATRIB_VOLUMEN:
                    match(TK_ATRIB_VOLUMEN);
                    parseOperComp();
                    match(TK_PORCENTAJE);
                    break;

                case TK_ATRIB_MUTE:
                    match(TK_ATRIB_MUTE);
                    parseOperIgualdad();
                    match(TK_BOOL_ACTUADOR);
                    break;

                case TK_ATRIB_MENSAJE:
                    match(TK_ATRIB_MENSAJE);
                    parseOperIgualdad();
                    match(TK_TEXTO);
                    break;

                case TK_ATRIB_EMAIL_NOTIF:
                    match(TK_ATRIB_EMAIL_NOTIF);
                    parseOperIgualdad();
                    match(TK_EMAIL);
                    break;

                default:
                    errorSintactico("Atributo inválido para ALTAVOZ");
            }
            break;

        case TK_ALARMA_ID:
            match(TK_ALARMA_ID);
            match(TK_DELIMITADOR);

            switch (lookahead.tipo)
            {
                case TK_ATRIB_ESTADO:
                    match(TK_ATRIB_ESTADO);
                    parseOperIgualdad();
                    match(TK_BOOL_ACTUADOR);
                    break;

                case TK_ATRIB_ACTIVADA:
                    match(TK_ATRIB_ACTIVADA);
                    parseOperIgualdad();
                    match(TK_BOOL_ACTUADOR);
                    break;

                default:
                    errorSintactico("Atributo inválido para ALARMA");
            }
            break;

        default:
            errorSintactico("Se esperaba un actuador");
    }
}

void parseOperComp(void)
{
    switch (lookahead.tipo)
    {
        case TK_MAYOR:
            match(TK_MAYOR);
            break;

        case TK_MENOR:
            match(TK_MENOR);
            break;

        case TK_MAYORIGUAL:
            match(TK_MAYORIGUAL);
            break;

        case TK_MENORIGUAL:
            match(TK_MENORIGUAL);
            break;

        default:
            errorSintactico("Se esperaba un operador de comparación");
    }
}

void parseOperIgualdad(void)
{
    switch (lookahead.tipo)
    {
        case TK_IGUAL:
            match(TK_IGUAL);
            break;

        case TK_DIFERENTE:
            match(TK_DIFERENTE);
            break;

        default:
            errorSintactico("Se esperaba un operador de igualdad (== o !=)");
    }
}

void parseAsignacion(void)
{
    switch (lookahead.tipo)
    {
        case TK_FOCO_ID:
            match(TK_FOCO_ID);
            match(TK_DELIMITADOR);

            switch (lookahead.tipo)
            {
                case TK_ATRIB_ESTADO:
                    match(TK_ATRIB_ESTADO);
                    match(TK_ASIGNACION);
                    match(TK_BOOL_ACTUADOR);
                    break;

                case TK_ATRIB_BRILLO:
                    match(TK_ATRIB_BRILLO);
                    match(TK_ASIGNACION);
                    match(TK_PORCENTAJE);
                    break;

                case TK_ATRIB_COLOR:
                    match(TK_ATRIB_COLOR);
                    match(TK_ASIGNACION);
                    match(TK_COLOR);
                    break;

                default:
                    errorSintactico("Atributo inválido para FOCO");
            }
            break;

        case TK_AIRE_ID:
            match(TK_AIRE_ID);
            match(TK_DELIMITADOR);

            switch (lookahead.tipo)
            {
                case TK_ATRIB_ESTADO:
                    match(TK_ATRIB_ESTADO);
                    match(TK_ASIGNACION);
                    match(TK_BOOL_ACTUADOR);
                    break;

                case TK_ATRIB_TEMP_O:
                    match(TK_ATRIB_TEMP_O);
                    match(TK_ASIGNACION);
                    match(TK_TEMP);
                    break;

                case TK_ATRIB_MODO:
                    match(TK_ATRIB_MODO);
                    match(TK_ASIGNACION);
                    match(TK_MODO);
                    break;

                default:
                    errorSintactico("Atributo inválido para AIRE");
            }
            break;

        case TK_PERSIANA_ID:
            match(TK_PERSIANA_ID);
            match(TK_DELIMITADOR);
            match(TK_ATRIB_POSICION);
            match(TK_ASIGNACION);
            match(TK_PORCENTAJE);
            break;

        case TK_CERRADURA_ID:
            match(TK_CERRADURA_ID);
            match(TK_DELIMITADOR);
            match(TK_ATRIB_ESTADO);
            match(TK_ASIGNACION);
            match(TK_BOOL_ACTUADOR);
            break;

        case TK_ALTAVOZ_ID:
            match(TK_ALTAVOZ_ID);
            match(TK_DELIMITADOR);

            switch (lookahead.tipo)
            {
                case TK_ATRIB_VOLUMEN:
                    match(TK_ATRIB_VOLUMEN);
                    match(TK_ASIGNACION);
                    match(TK_PORCENTAJE);
                    break;

                case TK_ATRIB_MUTE:
                    match(TK_ATRIB_MUTE);
                    match(TK_ASIGNACION);
                    match(TK_BOOL_ACTUADOR);
                    break;

                case TK_ATRIB_MENSAJE:
                    match(TK_ATRIB_MENSAJE);
                    match(TK_ASIGNACION);
                    match(TK_TEXTO);
                    break;

                case TK_ATRIB_EMAIL_NOTIF:
                    match(TK_ATRIB_EMAIL_NOTIF);
                    match(TK_ASIGNACION);
                    match(TK_EMAIL);
                    break;

                default:
                    errorSintactico("Atributo inválido para ALTAVOZ");
            }
            break;

        case TK_ALARMA_ID:
            match(TK_ALARMA_ID);
            match(TK_DELIMITADOR);

            switch (lookahead.tipo)
            {
                case TK_ATRIB_ESTADO:
                    match(TK_ATRIB_ESTADO);
                    match(TK_ASIGNACION);
                    match(TK_BOOL_ACTUADOR);
                    break;

                case TK_ATRIB_ACTIVADA:
                    match(TK_ATRIB_ACTIVADA);
                    match(TK_ASIGNACION);
                    match(TK_BOOL_ACTUADOR);
                    break;

                default:
                    errorSintactico("Atributo inválido para ALARMA");
            }
            break;

        default:
            errorSintactico("Se esperaba una asignación");
    }
}

