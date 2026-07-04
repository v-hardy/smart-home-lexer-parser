#include <stdio.h>
#include "ast.h"

int main() {

    // Literal: 10
    Token t10 = {.lexema = "10"};
    NodoAST *lit10 = crearLiteral(t10);

    // IF
    Token ifTok = {.lexema = "if"};
    NodoAST *ifNodo = crearIf(ifTok);

    ifNodo->ifNodo.condicion = lit10;

    // Bloque THEN
    ListaAST *thenList = NULL;
    thenList = agregarInstruccion(thenList, crearLiteral(t10));

    ifNodo->ifNodo.thenBloque = thenList;

    // PROGRAMA
    NodoAST *prog = crearPrograma();
    prog->programa.instrucciones = NULL;
    prog->programa.instrucciones =
        agregarInstruccion(prog->programa.instrucciones, ifNodo);

    // IMPRIMIR
    imprimirAST(prog);

    liberarAST(prog);
    return 0;
}