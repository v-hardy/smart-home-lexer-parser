#include <stdlib.h>
#include <string.h>

#include "estado.h"


/*==================== ESTADO ====================*/

void inicializarEstado(EstadoSistema *estado)
{
    estado->actuadores = NULL;
}


/*==================== ACTUADORES ====================*/

Actuador *buscarActuador(EstadoSistema *estado, const char *nombre)
{
    Actuador *actual = estado->actuadores;

    while (actual)
    {
        if (strcmp(actual->nombre, nombre) == 0)
            return actual;

        actual = actual->sig;
    }

    return NULL;
}


Actuador *crearActuador(EstadoSistema *estado, Token dispositivo)
{
    Actuador *nuevo = malloc(sizeof(Actuador));

    strcpy(nuevo->nombre, dispositivo.lexema);

    nuevo->tipo = dispositivo;

    nuevo->atributos = NULL;

    nuevo->sig = estado->actuadores;

    estado->actuadores = nuevo;

    return nuevo;
}


Actuador *obtenerActuador(EstadoSistema *estado, Token dispositivo)
{
    Actuador *act = buscarActuador(estado, dispositivo.lexema);

    if (act != NULL)
        return act;

    return crearActuador(estado, dispositivo);
}


/*==================== ATRIBUTOS ====================*/

Atributo *buscarAtributo(Actuador *actuador, const char *nombre)
{
    Atributo *actual = actuador->atributos;

    while (actual)
    {
        if (strcmp(actual->nombre, nombre) == 0)
            return actual;

        actual = actual->sig;
    }

    return NULL;
}


void establecerAtributo(Actuador *actuador, Token atributo, Token valor)
{
    Atributo *atr = buscarAtributo(actuador, atributo.lexema);

    if (atr == NULL)
    {
        atr = malloc(sizeof(Atributo));

        if (atr == NULL)
            return;

        strcpy(atr->nombre, atributo.lexema);

        atr->sig = actuador->atributos;
        actuador->atributos = atr;
    }

    atr->valor = valor;
}


/*==================== LIBERACION ====================*/

void destruirEstado(EstadoSistema *estado)
{
    Actuador *act = estado->actuadores;

    while (act)
    {
        Atributo *atr = act->atributos;

        while (atr)
        {
            Atributo *sigAtr = atr->sig;

            free(atr);

            atr = sigAtr;
        }

        Actuador *sigAct = act->sig;

        free(act);

        act = sigAct;
    }

    estado->actuadores = NULL;
}