#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "estado.h"

void interpretarPrograma(NodoAST *raiz, EstadoSistema *estado);

#endif