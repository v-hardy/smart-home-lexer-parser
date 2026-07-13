#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>


#include "interpreter.h"
#include "estado.h"

/*================ PROTOTIPOS ================*/

static const DefaultAtributo *buscarDefault(const char *disp, const char *atr);

static Atributo *crearAtributoVirtual(const char *dispositivo, const char *atributo);

void errorSintactico(const char *mensaje);



/*================ IMPLEMENTACIONES ================*/

Atributo *crearAtributoVirtual(const char *dispositivo, const char *atributo)
{
    static Atributo virtual;

    const DefaultAtributo *d = buscarDefault(dispositivo, atributo);

    if(d==NULL)
        return NULL;

    memset(&virtual,0,sizeof(Atributo));

    strcpy(virtual.nombre, atributo);

    virtual.valor.tipo = d->tipo;

    strcpy(virtual.valor.lexema, d->lexema);

    switch(d->tipo)
    {
        case TK_BOOL_ACTUADOR:
            virtual.valor.valor.booleano = d->valor.booleano;
            break;

        case TK_TEMP:
        case TK_PORCENTAJE:
        case TK_LUX:
            virtual.valor.valor.numero = d->valor.numero;
            break;

        default:
            strcpy(virtual.valor.valor.texto, d->valor.texto);
            break;
    }

    return &virtual;
}

Atributo *obtenerAtributo(EstadoSistema *estado, const char *dispositivo, const char *atributo)
{
    Actuador *act = buscarActuador(estado, dispositivo);

    if (act != NULL)
    {
        Atributo *atr = buscarAtributo(act, atributo);

        if (atr != NULL)
            return atr;

        /* El actuador existe, pero el atributo no */
        Atributo *def = crearAtributoVirtual(dispositivo, atributo);

        if (def != NULL)
        {
            printf("\n[INFO] El actuador \"%s\" no posee el atributo \"%s\".\n", dispositivo, atributo);

            printf("       Se utilizará el valor por defecto: %s = %s\n", def->nombre, def->valor.lexema);
        }

        return def;
    }

    /* El actuador no existe */
    Atributo *def = crearAtributoVirtual(dispositivo, atributo);

    if (def != NULL)
    {
        printf("\n[INFO] El actuador \"%s\" no existe.\n", dispositivo);

        printf("       Se utilizará el valor por defecto: %s = %s\n", def->nombre, def->valor.lexema);
    }

    return def;
}

static const DefaultAtributo *buscarDefault(const char *disp, const char *atr)
{
    size_t n = sizeof(defaults)/sizeof(defaults[0]);

    for(size_t i=0;i<n;i++)
    {
        if(strncmp(disp, defaults[i].prefijo, strlen(defaults[i].prefijo))==0 &&
           strcmp(atr,defaults[i].atributo)==0)
        {
            return &defaults[i];
        }
    }

    return NULL;
}


/*------------------------------------------
    Auxiliares de expresiones
-------------------------------------------*/

static int fechaAEntero(const char *fecha)
{
    int a, m, d;

    if(sscanf(fecha, "%d-%d-%d", &a, &m, &d) != 3)
        return -1;

    return a * 10000 + m * 100 + d;
}

static int horaAMinutos(const char *hora)
{
    int h, m;

    if(sscanf(hora, "%d:%d", &h, &m) != 2)
        return -1;

    return h * 60 + m;
}

static bool strAbool(Token t)
{
    if(strcmp(t.lexema, "TRUE") == 0 || strcmp(t.lexema, "ON") == 0)
        return true;

    if(strcmp(t.lexema, "FALSE") == 0 || strcmp(t.lexema, "OFF") == 0)
        return false;

    errorSintactico("Se esperaba str boleanizable)");
    return false;
}

static bool compararNumero(double lhs, double rhs, Token op)
{
    switch(op.tipo)
    {
        case TK_MAYOR:
            return lhs > rhs;

        case TK_MENOR:
            return lhs < rhs;

        case TK_MAYORIGUAL:
            return lhs >= rhs;

        case TK_MENORIGUAL:
            return lhs <= rhs;

        case TK_IGUAL:
            return lhs == rhs;

        case TK_DIFERENTE:
            return lhs != rhs;

        default:
            return false;
    }
}

static bool compararBool(bool lhs, bool rhs, Token op)
{
    switch(op.tipo)
    {
        case TK_IGUAL:
            return lhs == rhs;

        case TK_DIFERENTE:
            return lhs != rhs;

        default:
            return false;
    }
}

static bool compararTexto(const char *lhs, const char *rhs, Token op)
{
    switch(op.tipo)
    {
        case TK_IGUAL:
            return strcmp(lhs, rhs) == 0;

        case TK_DIFERENTE:
            return strcmp(lhs, rhs) != 0;

        default:
            return false;
    }
}


/*------------------------------------------
    Sensores simulados
-------------------------------------------*/

static float pedirFloat(const char *mensaje, float min, float max)
{
    char linea[100];
    float valor;

    while (1)
    {
        printf("%-22s [%.1f .. %.1f] : ", mensaje, min, max);

        if (fgets(linea, sizeof(linea), stdin) == NULL)
            continue;

        /* ENTER -> generar aleatorio */
        if (linea[0] == '\n')
        {
            valor = min + ((float)rand() / RAND_MAX) * (max - min);

            printf("   Valor generado: %.1f\n", valor);
            return valor;
        }

        if (sscanf(linea, "%f", &valor) != 1)
        {
            printf("   Error: formato inválido.\n");
            continue;
        }

        if (valor < min || valor > max)
        {
            printf("   Error: el valor debe estar entre %.1f y %.1f.\n", min, max);
            continue;
        }

        return valor;
    }
}

static int pedirInt(const char *mensaje, int min, int max)
{
    char linea[100];
    int valor;

    while (1)
    {
        printf("%-22s [%d .. %d] : ", mensaje, min, max);

        if (fgets(linea, sizeof(linea), stdin) == NULL)
            continue;

        /* ENTER -> generar aleatorio */
        if (linea[0] == '\n')
        {
            valor = min + rand() % (max - min + 1);

            printf("   Valor generado: %d\n", valor);
            return valor;
        }

        if (sscanf(linea, "%d", &valor) != 1)
        {
            printf("   Error: formato inválido.\n");
            continue;
        }

        if (valor < min || valor > max)
        {
            printf("   Error: el valor debe estar entre %d y %d.\n",
                   min, max);
            continue;
        }

        return valor;
    }
}

static bool pedirBool(const char *mensaje)
{
    char linea[100];
    int valor;

    while (1)
    {
        printf("%-22s (0=No, 1=Sí) : ", mensaje);

        if (fgets(linea, sizeof(linea), stdin) == NULL)
            continue;

        /* ENTER -> generar aleatorio */
        if (linea[0] == '\n')
        {
            valor = rand() % 2;

            printf("   Valor generado: %s\n", valor ? "Sí" : "No");

            return valor;
        }

        if (sscanf(linea, "%d", &valor) != 1)
        {
            printf("   Error: formato inválido.\n");
            continue;
        }

        if (valor != 0 && valor != 1)
        {
            printf("   Error: sólo se admite 0 o 1.\n");
            continue;
        }

        return valor == 1;
    }
}

static void cargarSensores(EstadoSistema *estado)
{
    srand((unsigned)time(NULL));

    printf("\n");
    printf("=====================================================\n");
    printf("           SIMULADOR SMART HOME\n");
    printf("=====================================================\n\n");

    printf("Ingrese el estado inicial de los sensores.\n");
    printf("Presione ENTER para generar un valor aleatorio.\n\n");

    estado->sensores.temperatura = pedirFloat("Temperatura (°C)", -10.0f, 50.0f);

    printf("\n");

    estado->sensores.humedad = pedirInt("Humedad (%)", 0, 100);
    
    printf("\n");

    estado->sensores.luz = pedirInt("Iluminancia (lux)", 0, 1000);

    printf("\n");

    estado->sensores.movimiento = pedirBool("¿Hay movimiento?");

    printf("\n");

    estado->sensores.humo = pedirBool("¿Hay humo?");

    printf("\n");
    printf("\n");
    printf("-----------------------------------------------------\n");
    printf("Sensores inicializados\n");
    printf("-----------------------------------------------------\n");

    printf("%-15s : %6.1f °C\n",  "Temperatura", estado->sensores.temperatura);
    printf("%-15s : %6d %%\n",    "Humedad",     estado->sensores.humedad);
    printf("%-15s : %6d lux\n",   "Luz",         estado->sensores.luz);
    printf("%-15s : %6s\n",       "Movimiento",  estado->sensores.movimiento ? "Si" : "No");
    printf("%-15s : %6s\n",       "Humo",        estado->sensores.humo ? "Si" : "No");

    printf("-----------------------------------------------------\n");
}

/*------------------------------------------
    Expresiones
-------------------------------------------*/

static bool evaluarSensorExpr(NodoAST *expr, EstadoSistema *estado)
{
    Token operador = expr->sensorExpr.operador;
    Token literal  = expr->sensorExpr.valor->lit.literal;

    switch(expr->sensorExpr.sensor.tipo)
    {
        /*-------------------------------------------*/
        /* NUMÉRICOS                                 */
        /*-------------------------------------------*/

        case TK_SENSOR_TEMP:
        {
            double lhs = estado->sensores.temperatura;
            double rhs = atof(literal.lexema);

            return compararNumero(lhs, rhs, operador);
        }

        case TK_SENSOR_HUMEDAD:
        {
            double lhs = estado->sensores.humedad;
            double rhs = atof(literal.lexema);

            return compararNumero(lhs, rhs, operador);
        }

        case TK_SENSOR_LUZ:
        {
            double lhs = estado->sensores.luz;
            double rhs = atof(literal.lexema);

            return compararNumero(lhs, rhs, operador);
        }

        /*-------------------------------------------*/
        /* BOOLEANOS                                 */
        /*-------------------------------------------*/

        case TK_SENSOR_MOVIMIENTO:
        {
            bool lhs = estado->sensores.movimiento;
            bool rhs = strAbool(literal);

            return compararBool(lhs, rhs, operador);
        }

        case TK_SENSOR_HUMO:
        {
            bool lhs = estado->sensores.humo;
            bool rhs = strAbool(literal);

            return compararBool(lhs, rhs, operador);
        }

        default:
            return false;
    }
}

static bool evaluarActuadorExpr(NodoAST *expr, EstadoSistema *estado)
{
    Token dispositivo = expr->actuadorExpr.dispositivo;
    Token atributo    = expr->actuadorExpr.atributo;
    Token operador    = expr->actuadorExpr.operador;
    Token literal     = expr->actuadorExpr.valor->lit.literal;

    Atributo *atr = obtenerAtributo(estado, dispositivo.lexema, atributo.lexema);

    if (atr == NULL)
    {
        // aca no deberia entrar si todo salio bien
        printf("Dispositivo o atributo desconocido\n");
        return false;
    }
    
    switch(atr->valor.tipo)
    {
        /*-------------------------------------------*/
        /* BOOLEANOS                                 */
        /*-------------------------------------------*/

        case TK_BOOL_ACTUADOR:
        {
            bool lhs = strAbool(atr->valor);
            bool rhs = strAbool(literal);

            return compararBool(lhs, rhs, operador);
        }

        /*-------------------------------------------*/
        /* NUMÉRICOS                                 */
        /*-------------------------------------------*/

        case TK_TEMP:
        case TK_PORCENTAJE:
        {
            double lhs = atof(atr->valor.lexema);
            double rhs = atof(literal.lexema);

            return compararNumero(lhs, rhs, operador);
        }

        case TK_HORA:
        {
            double lhs = horaAMinutos(atr->valor.lexema);
            double rhs = horaAMinutos(literal.lexema);

            return compararNumero(lhs, rhs, operador);
        }
        case TK_FECHA:
        {
            double lhs = fechaAEntero(atr->valor.lexema);
            double rhs = fechaAEntero(literal.lexema);

            return compararNumero(lhs, rhs, operador);
        }

        /*-------------------------------------------*/
        /* TEXTO                                     */
        /*-------------------------------------------*/

        case TK_COLOR:
        case TK_MODO:
        case TK_TEXTO:
        case TK_EMAIL:
        {
            return compararTexto(
                atr->valor.lexema,
                literal.lexema,
                operador
            );
        }

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

        case AST_ACTUADOR_EXPR:
            return evaluarActuadorExpr(cond, estado);
            
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