#ifndef ESTADO_H
#define ESTADO_H

#include <stdbool.h>
#include "token.h"

/*==================== SENSORES ====================*/

typedef struct
{
    float temperatura;

    int humedad;
    int luz;

    bool movimiento;
    bool humo;

} EstadoSensores;


/*==================== ATRIBUTOS ====================*/

typedef struct Atributo
{
    char nombre[64];
    Token valor;

    struct Atributo *sig;

} Atributo;


/*==================== ACTUADORES ====================*/

typedef struct Actuador
{
    char nombre[64];
    Token tipo;

    Atributo *atributos;

    struct Actuador *sig;

} Actuador;


/*==================== SISTEMA ====================*/

typedef struct
{
    EstadoSensores sensores;

    Actuador *actuadores;

} EstadoSistema;


/*==================== API ====================*/

void inicializarEstado(EstadoSistema *estado);

void destruirEstado(EstadoSistema *estado);

Actuador *buscarActuador(EstadoSistema *estado, const char *nombre);

Actuador *crearActuador(EstadoSistema *estado, Token dispositivo);

Actuador *obtenerActuador(EstadoSistema *estado, Token dispositivo);

Atributo *buscarAtributo(Actuador *actuador, const char *nombre);

void establecerAtributo(Actuador *actuador, Token atributo, Token valor);

#endif