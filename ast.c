#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

/* ===================== UTILIDAD INTERNA ===================== */

static NodoAST *crearNodo(TipoNodoAST tipo)
{
    NodoAST *nodo = malloc(sizeof(NodoAST));

    if (!nodo)
    {
        fprintf(stderr, "Error: memoria insuficiente.\n");
        exit(EXIT_FAILURE);
    }

    memset(nodo, 0, sizeof(NodoAST));
    nodo->tipo = tipo;

    return nodo;
}

/* ===================== CONSTRUCTORES ===================== */

NodoAST *crearPrograma(void)
{
    return crearNodo(AST_PROGRAMA);
}

NodoAST *crearLiteral(Token token)
{
    NodoAST *nodo = crearNodo(AST_LITERAL);

    nodo->token = token;
    nodo->lit.literal = token;

    return nodo;
}

NodoAST *crearWhen(Token token)
{
    NodoAST *nodo = crearNodo(AST_WHEN);
    nodo->token = token;
    return nodo;
}

NodoAST *crearEvery(Token tokenTiempo)
{
    NodoAST *nodo = crearNodo(AST_EVERY);

    nodo->token = tokenTiempo;
    nodo->every.tiempo = tokenTiempo;
    nodo->every.bloque = NULL;

    return nodo;
}

NodoAST *crearIf(Token token)
{
    NodoAST *nodo = crearNodo(AST_IF);

    nodo->token = token;
    nodo->ifNodo.condicion = NULL;
    nodo->ifNodo.thenBloque = NULL;
    nodo->ifNodo.elseBloque = NULL;

    return nodo;
}

NodoAST *crearNodoBinario(TipoNodoAST tipo, NodoAST *izq, NodoAST *der)
{
    NodoAST *nodo = crearNodo(tipo);

    nodo->bin.izq = izq;
    nodo->bin.der = der;

    return nodo;
}

NodoAST *crearNodoNot(NodoAST *expr)
{
    NodoAST *nodo = crearNodo(AST_NOT);

    nodo->un.expr = expr;

    return nodo;
}

NodoAST *crearAsignacion(Token dispositivo, Token atributo, NodoAST *valor)
{
    NodoAST *nodo = crearNodo(AST_ASIGNACION);

    nodo->token = dispositivo;
    nodo->actuadorExpr.dispositivo = dispositivo;
    nodo->actuadorExpr.atributo = atributo;
    nodo->actuadorExpr.valor = valor;

    return nodo;
}

NodoAST *crearSensorExpr(Token sensor, Token operador, NodoAST *valor)
{
    NodoAST *nodo = crearNodo(AST_SENSOR_EXPR);

    nodo->token = sensor;
    nodo->sensorExpr.sensor = sensor;
    nodo->sensorExpr.operador = operador;
    nodo->sensorExpr.valor = valor;

    return nodo;
}

NodoAST *crearActuadorExpr(Token dispositivo, Token atributo, Token operador, NodoAST *valor)
{
    NodoAST *nodo = crearNodo(AST_ACTUADOR_EXPR);

    nodo->token = dispositivo;
    nodo->actuadorExpr.dispositivo = dispositivo;
    nodo->actuadorExpr.atributo = atributo;
    nodo->actuadorExpr.operador = operador;
    nodo->actuadorExpr.valor = valor;

    return nodo;
}

/* ===================== LISTA ===================== */

ListaAST *agregarInstruccion(ListaAST *lista, NodoAST *nodo)
{
    ListaAST *nuevo = malloc(sizeof(ListaAST));

    if (!nuevo)
    {
        fprintf(stderr, "Error: memoria insuficiente en lista AST\n");
        exit(EXIT_FAILURE);
    }

    nuevo->nodo = nodo;
    nuevo->sig = NULL;

    if (lista == NULL)
        return nuevo;

    ListaAST *aux = lista;

    while (aux->sig != NULL)
        aux = aux->sig;

    aux->sig = nuevo;

    return lista;
}

/* ===================== IMPRESIÓN ===================== */

static void imprimirNodo(NodoAST *nodo, int nivel);

static void indent(int n)
{
    for (int i = 0; i < n; i++)
        printf("  ");
}

void imprimirAST(NodoAST *raiz)
{
    imprimirNodo(raiz, 0);
}

static void imprimirLista(ListaAST *lista, int nivel)
{
    while (lista)
    {
        imprimirNodo(lista->nodo, nivel);
        lista = lista->sig;
    }
}

static void imprimirNodo(NodoAST *nodo, int nivel)
{
    if (!nodo)
        return;

    indent(nivel);

    switch (nodo->tipo)
    {
        case AST_PROGRAMA:
            printf("PROGRAMA\n");
            imprimirLista((ListaAST *)nodo->programa.instrucciones, nivel + 1);
            break;

        case AST_LISTA:
            printf("LISTA\n");
            imprimirNodo(nodo->list.head, nivel + 1);
            imprimirNodo(nodo->list.next, nivel);
            break;

        case AST_IF:
            printf("IF\n");

            indent(nivel + 1);
            printf("condicion:\n");
            imprimirNodo(nodo->ifNodo.condicion, nivel + 2);

            indent(nivel + 1);
            printf("then:\n");
            imprimirLista(nodo->ifNodo.thenBloque, nivel + 2);

            if (nodo->ifNodo.elseBloque)
            {
                indent(nivel + 1);
                printf("else:\n");
                imprimirLista(nodo->ifNodo.elseBloque, nivel + 2);
            }
            break;

        case AST_WHEN:
            printf("WHEN\n");
            indent(nivel + 1);
            printf("condicion:\n");
            imprimirNodo(nodo->when.condicion, nivel + 2);

            indent(nivel + 1);
            printf("bloque:\n");
            imprimirNodo(nodo->when.bloque, nivel + 2);
            break;

        case AST_EVERY:
            printf("EVERY %s\n", nodo->every.tiempo.lexema);
            imprimirNodo(nodo->every.bloque, nivel + 1);
            break;

        case AST_ASIGNACION:
            printf("ASIGNACION %s.%s\n",
                   nodo->actuadorExpr.dispositivo.lexema,
                   nodo->actuadorExpr.atributo.lexema);

            imprimirNodo(nodo->actuadorExpr.valor, nivel + 1);
            break;

        case AST_SENSOR_EXPR:
            printf("SENSOR %s\n", nodo->sensorExpr.sensor.lexema);

            imprimirNodo(nodo->sensorExpr.valor, nivel + 1);
            break;

        case AST_ACTUADOR_EXPR:
            printf("ACTUADOR_EXPR %s.%s\n",
                   nodo->actuadorExpr.dispositivo.lexema,
                   nodo->actuadorExpr.atributo.lexema);

            imprimirNodo(nodo->actuadorExpr.valor, nivel + 1);
            break;

        case AST_LITERAL:
            printf("LITERAL: %s\n", nodo->lit.literal.lexema);
            break;

        case AST_OR:
        case AST_AND:
        case AST_NOT:
            printf("LOGICO\n");
            imprimirNodo(nodo->un.expr, nivel + 1);
            break;

        default:
            printf("Nodo desconocido\n");
            break;
    }
}

/* ===================== LIBERACIÓN ===================== */

void liberarAST(NodoAST *raiz)
{
    if (!raiz)
        return;

    switch (raiz->tipo)
    {
        case AST_IF:
            liberarAST(raiz->ifNodo.condicion);
            liberarAST((NodoAST *)raiz->ifNodo.thenBloque);
            liberarAST((NodoAST *)raiz->ifNodo.elseBloque);
            break;

        case AST_WHEN:
            liberarAST(raiz->when.condicion);
            liberarAST(raiz->when.bloque);
            break;

        case AST_EVERY:
            liberarAST(raiz->every.bloque);
            break;

        case AST_SENSOR_EXPR:
            liberarAST(raiz->sensorExpr.valor);
            break;

        case AST_ACTUADOR_EXPR:
        case AST_ASIGNACION:
            liberarAST(raiz->actuadorExpr.valor);
            break;

        case AST_OR:
        case AST_AND:
            liberarAST(raiz->bin.izq);
            liberarAST(raiz->bin.der);
            break;

        case AST_NOT:
            liberarAST(raiz->un.expr);
            break;

        default:
            break;
    }

    free(raiz);
}