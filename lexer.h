#ifndef LEXER_H
#define LEXER_H

#include "token.h"

extern Token lookahead;

int abrirFuente(const char *nombre);
void cerrarFuente(void);

void lexerInitDesdeString(const char *texto);

Token obtenerSiguienteToken(void);

void siguienteToken(void);

void match(TokenType esperado);

void errorSintactico(const char *);

void volcarTokens(void);

#endif