#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "estado.h"

typedef struct
{
    const char *prefijo;
    const char *atributo;

    TokenType tipo;
    const char *lexema;

    ValorToken valor;

} DefaultAtributo;

static const DefaultAtributo defaults[] =
{
    /*====================== FOCO ======================*/

    {"FOCO_", "ESTADO", TK_BOOL_ACTUADOR,
        "OFF", {.booleano = 0}},

    {"FOCO_", "BRILLO", TK_PORCENTAJE,
        "0", {.numero = 0}},

    {"FOCO_", "COLOR", TK_COLOR,
        "BLANCO", {.texto = "BLANCO"}},


    /*====================== AIRE ======================*/

    {"AIRE_", "ESTADO", TK_BOOL_ACTUADOR,
        "OFF", {.booleano = 0}},

    {"AIRE_", "MODO", TK_MODO,
        "FRIO", {.texto = "FRIO"}},

    {"AIRE_", "TEMP_OBJ", TK_TEMP,
        "24", {.numero = 24}},

    {"AIRE_", "TEMP_ACT", TK_TEMP,
        "24", {.numero = 24}},


    /*=================== PERSIANA =====================*/

    {"PERSIANA_", "POSICION", TK_PORCENTAJE,
        "0", {.numero = 0}},


    /*================== CERRADURA =====================*/

    {"CERRADURA_", "ESTADO", TK_BOOL_ACTUADOR,
        "OFF", {.booleano = 0}},


    /*===================== RELOJ ======================*/

    {"RELOJ_", "HORA", TK_HORA,
        "00:00", {.texto = "00:00"}},

    {"RELOJ_", "FECHA", TK_FECHA,
        "01/01/2000", {.texto = "01/01/2000"}},


    /*=================== ALTAVOZ ======================*/

    {"ALTAVOZ_", "VOLUMEN", TK_PORCENTAJE,
        "50", {.numero = 50}},

    {"ALTAVOZ_", "MUTE", TK_BOOL_ACTUADOR,
        "OFF", {.booleano = 0}},

    {"ALTAVOZ_", "MENSAJE", TK_TEXTO,
        "", {.texto = ""}},

    {"ALTAVOZ_", "EMAIL_NOTIF", TK_EMAIL,
        "", {.texto = ""}},


    /*===================== ALARMA =====================*/

    {"ALARMA_", "ESTADO", TK_BOOL_ACTUADOR,
        "OFF", {.booleano = 0}},

    {"ALARMA_", "ACTIVADA", TK_BOOL_ACTUADOR,
        "OFF", {.booleano = 0}}
};

void interpretarPrograma(NodoAST *raiz, EstadoSistema *estado);

#endif