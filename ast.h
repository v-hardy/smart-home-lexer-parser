#ifndef AST_H
#define AST_H

#include "token.h"


typedef struct ListaAST ListaAST;

/* ===================== TIPOS ===================== */

typedef enum
{
    AST_PROGRAMA,

    AST_WHEN,
    AST_EVERY,
    AST_IF,

    AST_ASIGNACION,

    AST_OR,
    AST_AND,
    AST_NOT,

    AST_SENSOR_EXPR,
    AST_ACTUADOR_EXPR,

    AST_LITERAL

} TipoNodoAST;

/* ===================== NODOS===================== */

typedef struct NodoAST NodoAST;

struct NodoAST
{
    TipoNodoAST tipo;
    Token token;

    union
    {
        struct
        {
            ListaAST *instrucciones;
        } programa;

        /* WHEN */
        struct
        {
            NodoAST *condicion;
            ListaAST *bloque;
        } when;

        /* EVERY */
        struct
        {
            Token tiempo;
            ListaAST *bloque;
        } every;

        /* IF */
        struct
        {
            NodoAST *condicion;
            ListaAST *thenBloque;
            ListaAST *elseBloque;
        } ifNodo;

        struct {
            Token sensor;
            Token operador;
            NodoAST *valor;
        } sensorExpr;

        struct {
            Token dispositivo;
            Token atributo;
            Token operador;
            NodoAST *valor;
        } actuadorExpr;

        /* BINARIOS (AND / OR) */
        struct
        {
            NodoAST *izq;
            NodoAST *der;
        } bin;

        /* NOT */
        struct
        {
            NodoAST *expr;
        } un;

        /* LITERAL */
        struct
        {
            Token literal;
        } lit;

        /* LISTA AST */
        struct
        {
            NodoAST *head;
            NodoAST *next;
        } list;
    };
};

/* ===================== LISTA EXTERNA ===================== */

typedef struct ListaAST
{
    NodoAST *nodo;
    struct ListaAST *sig;

} ListaAST;

/* ===================== CONSTRUCTORES ===================== */

NodoAST *crearPrograma(void);

NodoAST *crearWhen(Token token);

NodoAST *crearEvery(Token token);

NodoAST *crearIf(Token token);

NodoAST *crearAsignacion(Token dispositivo, Token atributo, NodoAST *valor);

NodoAST *crearLiteral(Token token);

NodoAST *crearSensorExpr(Token sensor, Token operador, NodoAST *valor);

NodoAST *crearActuadorExpr(Token dispositivo, Token atributo, Token operador, NodoAST *valor);

NodoAST *crearNodoBinario(TipoNodoAST tipo, NodoAST *izq, NodoAST *der);

NodoAST *crearNodoNot(NodoAST *expr);

/* LISTA */
ListaAST *agregarInstruccion(ListaAST *lista, NodoAST *nodo);

/* ===================== UTILIDADES ===================== */

void imprimirAST(NodoAST *raiz);

void liberarAST(NodoAST *raiz);

void liberarListaAST(ListaAST *lista);

#endif