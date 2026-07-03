#ifndef LEXER_H
#define LEXER_H

#include "token.h"

extern Token lookahead;

int abrirFuente(const char *nombre);
void cerrarFuente(void);

Token obtenerSiguienteToken(void);

void siguienteToken(void);

void match(TokenType esperado);

void errorSintactico(const char *);

void volcarTokens(void);

#endif