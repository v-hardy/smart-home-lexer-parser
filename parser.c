#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>


#include "config.h"
#include "ast.h"
#include "lexer.h"


// <======================================= Prototipos internos =======================================>

static NodoAST *parseInstruccion(void);
static bool iniciaInstruccion(TokenType t);

static NodoAST *parseBloqueWhen(void);
static NodoAST *parseBloqueEvery(void);
static NodoAST *parseBloqueCondicional(void);
static NodoAST *parseAsignacion(void);

static NodoAST *parseCondicion(void);
static NodoAST *parseOrExpr(void);
static NodoAST *parseAndExpr(void);
static NodoAST *parseNotExpr(void);

static NodoAST *parseExpresionLogica(void);
static NodoAST *parseSensorExpr(void);
static NodoAST *parseActuadorExpr(void);

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
NodoAST *parsePrograma(void)
{
    siguienteToken();

    ListaAST *listaPrograma = NULL;

    while (lookahead.tipo != TK_EOF)
    {
        NodoAST *instr = parseInstruccion();
        listaPrograma = agregarInstruccion(listaPrograma, instr);
    }

    match(TK_EOF);

    NodoAST *astRoot = crearPrograma();
    astRoot->programa.instrucciones = listaPrograma;

    return astRoot;
}

NodoAST *parseInstruccion(void)
{
    NodoAST *nodo = NULL;

    switch (lookahead.tipo)
    {
        case TK_WHEN:
            nodo = parseBloqueWhen();
            break;

        case TK_EVERY:
            nodo = parseBloqueEvery();
            break;

        case TK_IF:
            nodo = parseBloqueCondicional();
            break;

        case TK_FOCO_ID:
        case TK_AIRE_ID:
        case TK_PERSIANA_ID:
        case TK_CERRADURA_ID:
        case TK_RELOJ_ID:
        case TK_ALTAVOZ_ID:
        case TK_ALARMA_ID:
            if (iniciaInstruccion(lookahead.tipo))
                nodo = parseAsignacion();
            break;

        default:
            errorSintactico("Instrucción inválida");
    }
    return nodo;
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

NodoAST *parseBloqueWhen(void)
{
    match(TK_WHEN);

    NodoAST *node = crearWhen(lookahead);

    node->when.condicion = parseCondicion();

    match(TK_DO);

    ListaAST *bloque = NULL;

    while (iniciaInstruccion(lookahead.tipo))
    {
        NodoAST *stmt = parseInstruccion();
        bloque = agregarInstruccion(bloque, stmt);
    }

    node->when.bloque = bloque;

    match(TK_END);

    return node;
}

NodoAST *parseBloqueEvery(void)
{
    match(TK_EVERY);

    Token tiempo = lookahead;
    match(TK_TIEMPO);

    NodoAST *node = crearEvery(tiempo);

    match(TK_DO);

    ListaAST *bloque = NULL;

    while (iniciaInstruccion(lookahead.tipo))
    {
        NodoAST *stmt = parseInstruccion();
        bloque = agregarInstruccion(bloque, stmt);
    }

    node->every.bloque = bloque;

    match(TK_END);

    return node;
}

NodoAST *parseBloqueCondicional(void)
{
    match(TK_IF);

    NodoAST *ifNodo = crearIf(lookahead);

    ifNodo->ifNodo.condicion = parseCondicion();

    match(TK_THEN);

    ListaAST *thenList = NULL;

    while (iniciaInstruccion(lookahead.tipo))
    {
        NodoAST *stmt = parseInstruccion();
        thenList = agregarInstruccion(thenList, stmt);
    }

    ifNodo->ifNodo.thenBloque = thenList;

    if (lookahead.tipo == TK_ELSE)
    {
        match(TK_ELSE);

        ListaAST *elseList = NULL;

        while (iniciaInstruccion(lookahead.tipo))
        {
            NodoAST *stmt = parseInstruccion();
            elseList = agregarInstruccion(elseList, stmt);
        }

        ifNodo->ifNodo.elseBloque = elseList;
    }

    match(TK_END);

    return ifNodo;
}

NodoAST *parseCondicion(void)
{
    return parseOrExpr();
}

NodoAST *parseOrExpr(void)
{
    NodoAST *left = parseAndExpr();

    while (lookahead.tipo == TK_OR)
    {
        match(TK_OR);
        NodoAST *right = parseAndExpr();
        left = crearNodoBinario(AST_OR, left, right);
    }

    return left;
}

NodoAST *parseAndExpr(void)
{
    NodoAST *left = parseNotExpr();

    while (lookahead.tipo == TK_AND)
    {
        match(TK_AND);
        NodoAST *right = parseNotExpr();
        left = crearNodoBinario(AST_AND, left, right);
    }

    return left;
}

NodoAST *parseNotExpr(void)
{
    if (lookahead.tipo == TK_NOT)
    {
        match(TK_NOT);
        NodoAST *expr = parseNotExpr();
        return crearNodoNot(expr);
    }

    if (lookahead.tipo == TK_PAR_IZQ)
    {
        match(TK_PAR_IZQ);
        NodoAST *expr = parseCondicion();
        match(TK_PAR_DER);
        return expr;
    }

    return parseExpresionLogica();
}

NodoAST *parseExpresionLogica(void)
{
    switch(lookahead.tipo)
    {
        case TK_SENSOR_TEMP:
        case TK_SENSOR_HUMEDAD:
        case TK_SENSOR_LUZ:
        case TK_SENSOR_MOVIMIENTO:
        case TK_SENSOR_HUMO:
            return parseSensorExpr();

        case TK_FOCO_ID:
        case TK_AIRE_ID:
        case TK_PERSIANA_ID:
        case TK_CERRADURA_ID:
        case TK_RELOJ_ID:
        case TK_ALTAVOZ_ID:
        case TK_ALARMA_ID:
            return parseActuadorExpr();

        default:
            errorSintactico("Se esperaba una expresión lógica");
            return NULL;
    }
}

NodoAST *parseSensorExpr(void)
{
    Token sensor = lookahead;
    Token operador;
    Token valor;
    NodoAST *nodo = NULL;

    switch(lookahead.tipo)
    {
        case TK_SENSOR_TEMP:
            match(TK_SENSOR_TEMP);

            operador = lookahead;
            parseOperComp();

            valor = lookahead;
            match(TK_TEMP);

            nodo = crearSensorExpr(sensor, operador, NULL);
            nodo->sensorExpr.valor = crearLiteral(valor);
            break;

        case TK_SENSOR_HUMEDAD:
            match(TK_SENSOR_HUMEDAD);

            operador = lookahead;
            parseOperComp();

            valor = lookahead;
            match(TK_PORCENTAJE);

            nodo = crearSensorExpr(sensor, operador, NULL);
            nodo->sensorExpr.valor = crearLiteral(valor);
            break;

        case TK_SENSOR_LUZ:
            match(TK_SENSOR_LUZ);

            operador = lookahead;
            parseOperComp();

            valor = lookahead;
            match(TK_LUX);

            nodo = crearSensorExpr(sensor, operador, NULL);
            nodo->sensorExpr.valor = crearLiteral(valor);
            break;

        case TK_SENSOR_MOVIMIENTO:
            match(TK_SENSOR_MOVIMIENTO);

            operador = lookahead;
            parseOperIgualdad();

            valor = lookahead;
            match(TK_BOOL_SENSOR);

            nodo = crearSensorExpr(sensor, operador, NULL);
            nodo->sensorExpr.valor = crearLiteral(valor);
            break;

        case TK_SENSOR_HUMO:
            match(TK_SENSOR_HUMO);

            operador = lookahead;
            parseOperIgualdad();

            valor = lookahead;
            match(TK_BOOL_SENSOR);

            nodo = crearSensorExpr(sensor, operador, NULL);
            nodo->sensorExpr.valor = crearLiteral(valor);
            break;

        default:
            errorSintactico("Sensor inválido");
    }

    return nodo;
}

NodoAST *parseActuadorExpr(void)
{

    NodoAST *lastExpr = NULL;

    Token dispositivo = lookahead;

    switch (lookahead.tipo)
    {
        case TK_FOCO_ID:
            match(TK_FOCO_ID);
            match(TK_DELIMITADOR);

            {
                Token atributo;
                Token operador;
                Token valor;
                NodoAST *nodo;

                switch (lookahead.tipo)
                {
                    case TK_ATRIB_ESTADO:
                        atributo = lookahead;
                        match(TK_ATRIB_ESTADO);

                        operador = lookahead;
                        parseOperIgualdad();

                        valor = lookahead;
                        match(TK_BOOL_ACTUADOR);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    case TK_ATRIB_BRILLO:
                        atributo = lookahead;
                        match(TK_ATRIB_BRILLO);

                        operador = lookahead;
                        parseOperComp();

                        valor = lookahead;
                        match(TK_PORCENTAJE);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    case TK_ATRIB_COLOR:
                        atributo = lookahead;
                        match(TK_ATRIB_COLOR);

                        operador = lookahead;
                        parseOperIgualdad();

                        valor = lookahead;
                        match(TK_COLOR);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    default:
                        errorSintactico("Atributo inválido para FOCO");
                }
            }
            break;

        case TK_AIRE_ID:
            match(TK_AIRE_ID);
            match(TK_DELIMITADOR);

            {
                Token atributo;
                Token operador;
                Token valor;
                NodoAST *nodo;

                switch (lookahead.tipo)
                {
                    case TK_ATRIB_ESTADO:
                        atributo = lookahead;
                        match(TK_ATRIB_ESTADO);

                        operador = lookahead;
                        parseOperIgualdad();

                        valor = lookahead;
                        match(TK_BOOL_ACTUADOR);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    case TK_ATRIB_TEMP_O:
                        atributo = lookahead;
                        match(TK_ATRIB_TEMP_O);

                        operador = lookahead;
                        parseOperComp();

                        valor = lookahead;
                        match(TK_TEMP);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    case TK_ATRIB_TEMP_A:
                        atributo = lookahead;
                        match(TK_ATRIB_TEMP_A);

                        operador = lookahead;
                        parseOperComp();

                        valor = lookahead;
                        match(TK_TEMP);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    case TK_ATRIB_MODO:
                        atributo = lookahead;
                        match(TK_ATRIB_MODO);

                        operador = lookahead;
                        parseOperIgualdad();

                        valor = lookahead;
                        match(TK_MODO);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    default:
                        errorSintactico("Atributo inválido para AIRE");
                }
            }
            break;

        case TK_PERSIANA_ID:
            match(TK_PERSIANA_ID);
            match(TK_DELIMITADOR);

            {
                Token atributo = lookahead;
                Token operador;
                Token valor;
                NodoAST *nodo;

                switch (lookahead.tipo)
                {
                    case TK_ATRIB_POSICION:
                        match(TK_ATRIB_POSICION);

                        operador = lookahead;
                        parseOperComp();

                        valor = lookahead;
                        match(TK_PORCENTAJE);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    default:
                        errorSintactico("Atributo inválido para PERSIANA");
                }
            }
            break;

        case TK_CERRADURA_ID:
            match(TK_CERRADURA_ID);
            match(TK_DELIMITADOR);

            {
                Token atributo;
                Token operador;
                Token valor;
                NodoAST *nodo;

                switch (lookahead.tipo)
                {
                    case TK_ATRIB_ESTADO:
                        atributo = lookahead;
                        match(TK_ATRIB_ESTADO);

                        operador = lookahead;
                        parseOperIgualdad();

                        valor = lookahead;
                        match(TK_BOOL_ACTUADOR);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    default:
                        errorSintactico("Atributo inválido para CERRADURA");
                }
            }
            break;

        case TK_RELOJ_ID:
            match(TK_RELOJ_ID);
            match(TK_DELIMITADOR);

            {
                Token atributo;
                Token operador;
                Token valor;
                NodoAST *nodo;

                switch (lookahead.tipo)
                {
                    case TK_ATRIB_HORA:
                        atributo = lookahead;
                        match(TK_ATRIB_HORA);

                        operador = lookahead;
                        parseOperComp();

                        valor = lookahead;
                        match(TK_HORA);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    case TK_ATRIB_FECHA:
                        atributo = lookahead;
                        match(TK_ATRIB_FECHA);

                        operador = lookahead;
                        parseOperIgualdad();

                        valor = lookahead;
                        match(TK_FECHA);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    default:
                        errorSintactico("Atributo inválido para RELOJ");
                }
            }
            break;

        case TK_ALTAVOZ_ID:
            match(TK_ALTAVOZ_ID);
            match(TK_DELIMITADOR);

            {
                Token atributo;
                Token operador;
                Token valor;
                NodoAST *nodo;

                switch (lookahead.tipo)
                {
                    case TK_ATRIB_VOLUMEN:
                        atributo = lookahead;
                        match(TK_ATRIB_VOLUMEN);

                        operador = lookahead;
                        parseOperComp();

                        valor = lookahead;
                        match(TK_PORCENTAJE);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    case TK_ATRIB_MUTE:
                        atributo = lookahead;
                        match(TK_ATRIB_MUTE);

                        operador = lookahead;
                        parseOperIgualdad();

                        valor = lookahead;
                        match(TK_BOOL_ACTUADOR);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    case TK_ATRIB_MENSAJE:
                        atributo = lookahead;
                        match(TK_ATRIB_MENSAJE);

                        operador = lookahead;
                        parseOperIgualdad();

                        valor = lookahead;
                        match(TK_TEXTO);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    case TK_ATRIB_EMAIL_NOTIF:
                        atributo = lookahead;
                        match(TK_ATRIB_EMAIL_NOTIF);

                        operador = lookahead;
                        parseOperIgualdad();

                        valor = lookahead;
                        match(TK_EMAIL);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    default:
                        errorSintactico("Atributo inválido para ALTAVOZ");
                }
            }
            break;

        case TK_ALARMA_ID:
            match(TK_ALARMA_ID);
            match(TK_DELIMITADOR);

            {
                Token atributo;
                Token operador;
                Token valor;
                NodoAST *nodo;

                switch (lookahead.tipo)
                {
                    case TK_ATRIB_ESTADO:
                        atributo = lookahead;
                        match(TK_ATRIB_ESTADO);

                        operador = lookahead;
                        parseOperIgualdad();

                        valor = lookahead;
                        match(TK_BOOL_ACTUADOR);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    case TK_ATRIB_ACTIVADA:
                        atributo = lookahead;
                        match(TK_ATRIB_ACTIVADA);

                        operador = lookahead;
                        parseOperIgualdad();

                        valor = lookahead;
                        match(TK_BOOL_ACTUADOR);

                        nodo = crearActuadorExpr(dispositivo, atributo, operador, NULL);
                        nodo->actuadorExpr.valor = crearLiteral(valor);
                        lastExpr = nodo;
                        break;

                    default:
                        errorSintactico("Atributo inválido para ALARMA");
                }
            }
            break;

        default:
            errorSintactico("Se esperaba un actuador");
    }

    return lastExpr;
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

NodoAST *parseAsignacion(void)
{
    Token dispositivo = lookahead;
    Token atributo;
    Token valor;
    NodoAST *nodo = NULL;

    switch (lookahead.tipo)
    {
        case TK_FOCO_ID:
            match(TK_FOCO_ID);
            match(TK_DELIMITADOR);

            switch (lookahead.tipo)
            {
                case TK_ATRIB_ESTADO:
                    atributo = lookahead;
                    match(TK_ATRIB_ESTADO);
                    match(TK_ASIGNACION);
                    valor = lookahead;
                    match(TK_BOOL_ACTUADOR);

                    nodo = crearAsignacion(dispositivo, atributo, crearLiteral(valor));
                    break;

                case TK_ATRIB_BRILLO:
                    atributo = lookahead;
                    match(TK_ATRIB_BRILLO);
                    match(TK_ASIGNACION);
                    valor = lookahead;
                    match(TK_PORCENTAJE);

                    nodo = crearAsignacion(dispositivo, atributo, crearLiteral(valor));
                    break;

                case TK_ATRIB_COLOR:
                    atributo = lookahead;
                    match(TK_ATRIB_COLOR);
                    match(TK_ASIGNACION);
                    valor = lookahead;
                    match(TK_COLOR);

                    nodo = crearAsignacion(dispositivo, atributo, crearLiteral(valor));
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
                    atributo = lookahead;
                    match(TK_ATRIB_ESTADO);
                    match(TK_ASIGNACION);
                    valor = lookahead;
                    match(TK_BOOL_ACTUADOR);

                    nodo = crearAsignacion(dispositivo, atributo, crearLiteral(valor));
                    break;

                case TK_ATRIB_TEMP_O:
                    atributo = lookahead;
                    match(TK_ATRIB_TEMP_O);
                    match(TK_ASIGNACION);
                    valor = lookahead;
                    match(TK_TEMP);

                    nodo = crearAsignacion(dispositivo, atributo, crearLiteral(valor));
                    break;

                case TK_ATRIB_MODO:
                    atributo = lookahead;
                    match(TK_ATRIB_MODO);
                    match(TK_ASIGNACION);
                    valor = lookahead;
                    match(TK_MODO);

                    nodo = crearAsignacion(dispositivo, atributo, crearLiteral(valor));
                    break;

                default:
                    errorSintactico("Atributo inválido para AIRE");
            }
            break;

        case TK_PERSIANA_ID:
            match(TK_PERSIANA_ID);
            match(TK_DELIMITADOR);

            atributo = lookahead;
            match(TK_ATRIB_POSICION);
            match(TK_ASIGNACION);
            valor = lookahead;
            match(TK_PORCENTAJE);

            nodo = crearAsignacion(dispositivo, atributo, crearLiteral(valor));
            break;

        case TK_CERRADURA_ID:
            match(TK_CERRADURA_ID);
            match(TK_DELIMITADOR);

            atributo = lookahead;
            match(TK_ATRIB_ESTADO);
            match(TK_ASIGNACION);
            valor = lookahead;
            match(TK_BOOL_ACTUADOR);

            nodo = crearAsignacion(dispositivo, atributo, crearLiteral(valor));
            break;

        case TK_ALTAVOZ_ID:
            match(TK_ALTAVOZ_ID);
            match(TK_DELIMITADOR);

            switch (lookahead.tipo)
            {
                case TK_ATRIB_VOLUMEN:
                    atributo = lookahead;
                    match(TK_ATRIB_VOLUMEN);
                    match(TK_ASIGNACION);
                    valor = lookahead;
                    match(TK_PORCENTAJE);

                    nodo = crearAsignacion(dispositivo, atributo, crearLiteral(valor));
                    break;

                case TK_ATRIB_MUTE:
                    atributo = lookahead;
                    match(TK_ATRIB_MUTE);
                    match(TK_ASIGNACION);
                    valor = lookahead;
                    match(TK_BOOL_ACTUADOR);

                    nodo = crearAsignacion(dispositivo, atributo, crearLiteral(valor));
                    break;

                case TK_ATRIB_MENSAJE:
                    atributo = lookahead;
                    match(TK_ATRIB_MENSAJE);
                    match(TK_ASIGNACION);
                    valor = lookahead;
                    match(TK_TEXTO);

                    nodo = crearAsignacion(dispositivo, atributo, crearLiteral(valor));
                    break;

                case TK_ATRIB_EMAIL_NOTIF:
                    atributo = lookahead;
                    match(TK_ATRIB_EMAIL_NOTIF);
                    match(TK_ASIGNACION);
                    valor = lookahead;
                    match(TK_EMAIL);

                    nodo = crearAsignacion(dispositivo, atributo, crearLiteral(valor));
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
                    atributo = lookahead;
                    match(TK_ATRIB_ESTADO);
                    match(TK_ASIGNACION);
                    valor = lookahead;
                    match(TK_BOOL_ACTUADOR);

                    nodo = crearAsignacion(dispositivo, atributo, crearLiteral(valor));
                    break;

                case TK_ATRIB_ACTIVADA:
                    atributo = lookahead;
                    match(TK_ATRIB_ACTIVADA);
                    match(TK_ASIGNACION);
                    valor = lookahead;
                    match(TK_BOOL_ACTUADOR);

                    nodo = crearAsignacion(dispositivo, atributo, crearLiteral(valor));
                    break;

                default:
                    errorSintactico("Atributo inválido para ALARMA");
            }
            break;

        default:
            errorSintactico("Se esperaba una asignación");
    }

    return nodo;
}
