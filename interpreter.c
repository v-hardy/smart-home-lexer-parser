#include <string.h>
#include <stdlib.h>

#include "interpreter.h"

void errorSintactico(const char *mensaje);

static bool parseBool(Token t)
{
    if(strcmp(t.lexema, "TRUE") == 0)
        return true;

    if(strcmp(t.lexema, "FALSE") == 0)
        return false;

    errorSintactico("Se esperaba TRUE/FALSE)");
    return false;
}

/*------------------------------------------
    Sensores simulados
-------------------------------------------*/

static void cargarSensores(EstadoSistema *estado)
{
    estado->actuadores = NULL;

    estado->sensores.temperatura = 27.0;
    estado->sensores.humedad = 45.0;
    estado->sensores.luz = 100.0;
    estado->sensores.movimiento = true;
    estado->sensores.humo = true;
}


/*------------------------------------------
    Expresiones
-------------------------------------------*/

static bool evaluarSensorExpr(NodoAST *expr, EstadoSistema *estado)
{
    double sensor = 0;

    switch(expr->sensorExpr.sensor.tipo)
    {
        case TK_SENSOR_TEMP:
            sensor = estado->sensores.temperatura;
            break;

        case TK_SENSOR_HUMEDAD:
            sensor = estado->sensores.humedad;
            break;

        case TK_SENSOR_LUZ:
            sensor = estado->sensores.luz;
            break;

        case TK_SENSOR_MOVIMIENTO:
            sensor = estado->sensores.movimiento ? 1 : 0;
            break;

        case TK_SENSOR_HUMO:
            sensor = estado->sensores.humo ? 1 : 0;
            break;

        default:
            return false;
    }

    Token op = expr->sensorExpr.operador;
    Token valor = expr->sensorExpr.valor->lit.literal;

    double rhs;

    // detectar booleanos o números
    if(strcmp(valor.lexema, "TRUE") == 0 || strcmp(valor.lexema, "FALSE") == 0)
    {
        rhs = parseBool(valor) ? 1 : 0;
    }
    else
    {
        rhs = atof(valor.lexema);
    }

    switch(op.tipo)
    {
        case TK_MAYOR:
            return sensor > rhs;

        case TK_MENOR:
            return sensor < rhs;

        case TK_MAYORIGUAL:
            return sensor >= rhs;

        case TK_MENORIGUAL:
            return sensor <= rhs;

        case TK_IGUAL:
            return sensor == rhs;

        case TK_DIFERENTE:
            return sensor != rhs;

        default:
            return false;
    }
}

static bool evaluarCondicion(NodoAST *cond, EstadoSistema *estado)
{
    switch(cond->tipo)
    {
        case AST_SENSOR_EXPR:
            return evaluarSensorExpr(cond, estado);

        case AST_AND:
            return evaluarCondicion(cond->bin.izq, estado)
                && evaluarCondicion(cond->bin.der, estado);

        case AST_OR:
            return evaluarCondicion(cond->bin.izq, estado)
                || evaluarCondicion(cond->bin.der, estado);

        case AST_NOT:
            return !evaluarCondicion(cond->un.expr, estado);

        default:
            return false;
    }
}

/*------------------------------------------
    Asignaciones
-------------------------------------------*/

static void ejecutarAsignacion(NodoAST *nodo, EstadoSistema *estado)
{
    Actuador *act = obtenerActuador(estado, nodo->actuadorExpr.dispositivo);

    establecerAtributo(act, nodo->actuadorExpr.atributo, nodo->actuadorExpr.valor->lit.literal);
}

/*------------------------------------------
    Instrucciones
-------------------------------------------*/

static void ejecutarLista(ListaAST *lista, EstadoSistema *estado);

static void ejecutarNodo(NodoAST *nodo, EstadoSistema *estado)
{
    if(nodo == NULL)
        return;

    switch(nodo->tipo)
    {
        case AST_ASIGNACION:
            ejecutarAsignacion(nodo, estado);
            break;

        case AST_WHEN:

            if(evaluarCondicion(nodo->when.condicion, estado))
                ejecutarLista(nodo->when.bloque, estado);

            break;

        case AST_IF:

            if(evaluarCondicion(nodo->ifNodo.condicion, estado))
                ejecutarLista(nodo->ifNodo.thenBloque, estado);
            else
                ejecutarLista(nodo->ifNodo.elseBloque, estado);

            break;

        case AST_EVERY:

            /*
                El TPI no exige simular tiempo real.
                Ejecutamos el bloque una vez.
            */

            ejecutarLista(nodo->every.bloque, estado);

            break;

        default:
            break;
    }
}

static void ejecutarLista(ListaAST *lista, EstadoSistema *estado)
{
    while(lista)
    {
        ejecutarNodo(lista->nodo, estado);
        lista = lista->sig;
    }
}

/*------------------------------------------
    Entrada
-------------------------------------------*/

void interpretarPrograma(NodoAST *raiz, EstadoSistema *estado)
{
    if(raiz == NULL)
        return;

    inicializarEstado(estado);

    cargarSensores(estado);

    ejecutarLista(raiz->programa.instrucciones, estado);
    
}