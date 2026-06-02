#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


// <======================================= Definiciones de tipo de datos =======================================>

typedef enum {
    TK_ERROR,

    /* Palabras reservadas */
    TK_WHEN,
    TK_DO,
    TK_END,
    TK_EVERY,
    TK_IF,
    TK_THEN,
    TK_ELSE,

    /* Parentesis */
    TK_PAR_IZQ,  
    TK_PAR_DER,  

    /* Operadores Logicos */
    TK_AND,
    TK_OR,
    TK_NOT,

    /* Operadores de comparacion */
    TK_IGUAL,
    TK_DIFERENTE,
    TK_MAYOR,
    TK_MAYORIGUAL,
    TK_MENOR,
    TK_MENORIGUAL,
    
    /* Operador de asignacion */
    TK_ASIGNACION,     

    /* Delimitador */
    TK_DELIMITADOR,    

    /* Sensores */
    TK_SENSOR_TEMP,
    TK_SENSOR_HUMEDAD,
    TK_SENSOR_LUZ,
    TK_SENSOR_MOVIMIENTO,
    TK_SENSOR_HUMO,
    
    /* Dispositivos */
    TK_FOCO_ID,
    TK_AIRE_ID,
    TK_PERSIANA_ID,
    TK_CERRADURA_ID,
    TK_RELOJ_ID,
    TK_ALTAVOZ_ID,
    TK_ALARMA_ID,

    /* Atributos */
    TK_ATRIB_ESTADO,
    TK_ATRIB_BRILLO,
    TK_ATRIB_COLOR,
    TK_ATRIB_TEMP_O,
    TK_ATRIB_TEMP_A,
    TK_ATRIB_MODO,
    TK_ATRIB_POSICION,
    TK_ATRIB_HORA,
    TK_ATRIB_FECHA,
    TK_ATRIB_MUTE,
    TK_ATRIB_VOLUMEN,
    TK_ATRIB_EMAIL_NOTIF,
    TK_ATRIB_MENSAJE,
    TK_ATRIB_ACTIVADA,

    /* Literales */
    //TK_BOOL,      // TRUE=ON, FALSE=OFF <----
    TK_BOOL_SENSOR,     // TRUE, FALSE <---- ESTO HABRIA QUE CAMBIAR/AGREGAR EN LA GRAMATICA
    TK_BOOL_ACTUADOR,       // ON, OFF <----
    TK_TEMP,
    TK_PORCENTAJE,
    TK_TIEMPO,
    TK_LUX,
    TK_HORA,
    TK_FECHA,
    TK_TEXTO,
    TK_EMAIL,
    TK_MODO,
    TK_COLOR,

    TK_EOF
} TokenType;

// <======================================= Palabras Reservadas =======================================>

typedef struct {
    const char *lexema;
    TokenType tipo;
} Keyword;

Keyword keywords[] = {
    {"WHEN", TK_WHEN},
    {"DO", TK_DO},
    {"END", TK_END},
    {"IF", TK_IF},
    {"THEN", TK_THEN},
    {"ELSE", TK_ELSE},
    {"AND", TK_AND},
    {"OR", TK_OR},
    {"NOT", TK_NOT},

    {"TRUE", TK_BOOL_SENSOR},
    {"FALSE", TK_BOOL_SENSOR},
    {"ON", TK_BOOL_ACTUADOR},
    {"OFF", TK_BOOL_ACTUADOR},

    /* Sensores */
    {"SENSOR_TEMP", TK_SENSOR_TEMP},
    {"SENSOR_HUMEDAD", TK_SENSOR_HUMEDAD},
    {"SENSOR_LUZ", TK_SENSOR_LUZ},
    {"SENSOR_MOVIMIENTO", TK_SENSOR_MOVIMIENTO},
    {"SENSOR_HUMO", TK_SENSOR_HUMO},

    /* Atributos */
    {"ESTADO", TK_ATRIB_ESTADO},
    {"BRILLO", TK_ATRIB_BRILLO},
    {"COLOR", TK_ATRIB_COLOR},
    {"TEMP_OBJ", TK_ATRIB_TEMP_O},
    {"TEMP_ACT", TK_ATRIB_TEMP_A},
    {"MODO", TK_ATRIB_MODO},
    {"POSICION", TK_ATRIB_POSICION},
    {"HORA", TK_ATRIB_HORA},
    {"FECHA", TK_ATRIB_FECHA},
    {"MUTE", TK_ATRIB_MUTE},
    {"VOLUMEN", TK_ATRIB_VOLUMEN},
    {"EMAIL_NOTIF", TK_ATRIB_EMAIL_NOTIF},
    {"MENSAJE", TK_ATRIB_MENSAJE},
    {"ACTIVADA", TK_ATRIB_ACTIVADA},

};


// <======================================= Identificadores de dispositivos =======================================>




// <======================================= Literales y Unidades (Tokens Compuestos) =======================================>

typedef union {
    double numero;
    int    booleano;
    char   texto[64];
} ValorToken;

typedef struct {
    TokenType tipo;
    char lexema[64];

    ValorToken valor;

    int linea;
    int columna;
} Token;


// <======================================= Prototipos =======================================>

int   finDeArchivo(void);
void  errorSintactico(const char *mensaje);
void  parseInstruccion(void);
bool  iniciaInstruccion(TokenType t);
void  parseBloqueWhen(void);
void  parseBloqueEvery(void);
void  parseBloqueCondicional(void);
void  parseAsignacion(void);


// <======================================= Variables Globales =======================================>

Token lookahead;

static FILE *fuente = NULL;

static int caracterActual;
static int lineaActual = 1;
static int columnaActual = 0;


// <======================================= Funciones =======================================>

//* Abrir archivo */
int abrirFuente(const char *nombreArchivo)
{
    fuente = fopen(nombreArchivo, "r");

    if (fuente == NULL)
        return 0;

    caracterActual = fgetc(fuente);

    return 1;
}

/* Cerrar archivo */
void cerrarFuente(void)
{
    if (fuente != NULL)
    {
        fclose(fuente);
        fuente = NULL;
    }
}

/* Avanzar un caracter */
void avanzarCaracter(void)
{
    if (caracterActual == '\n')
    {
        lineaActual++;
        columnaActual = 0;
    }
    else
    {
        columnaActual++;
    }

    caracterActual = fgetc(fuente);
}

/* Obtiene caracter actual */
int obtenerCaracterActual(void)
{
    return caracterActual;
}

/* Omitir espacios */
void omitirEspacios(void)
{
    while (!finDeArchivo() &&
           isspace(caracterActual))
    {
        avanzarCaracter();
    }
}

/* Verificador de final de archivo */
int finDeArchivo(void)
{
    return caracterActual == EOF;
}

/* Creador token EOF */
Token crearTokenEOF(void)
{
    Token tk;

    tk.tipo = TK_EOF;
    tk.lexema[0] = '\0';
    tk.linea = lineaActual;
    tk.columna = columnaActual;

    return tk;
}

/* Obetener siguiente token */
Token obtenerSiguienteToken(void)
{
    omitirEspacios();

    if (finDeArchivo())
        return crearTokenEOF();

    Token tk;

    tk.linea = lineaActual;
    tk.columna = columnaActual;

    /*
       agregar llamado a funciones de reconocimiento de:
       - palabras reservadas
       - identificadores
       - operadores
       - literales
       - etc.
    */

    return tk;
}

/* Asignacion de siguiente token para ver si luego matchea con lo esperado */
void siguienteToken(void)
{
    lookahead = obtenerSiguienteToken();
}

/* Reporte de error sintactico con posicion */
void errorSintactico(const char *mensaje)
{
    fprintf(stderr, "Error sintáctico [línea %d, col %d]: %s\n",
            lineaActual, columnaActual, mensaje);
    exit(EXIT_FAILURE);
}

/* Coincidencia del siguiente con lo esperado */
void match(TokenType esperado) {
    if (lookahead.tipo == esperado) {
        siguienteToken();
    } else {
        errorSintactico("Token inesperado");
    }
}

/* Buscar en palabras reservadas */
TokenType buscarKeyword(const char *lexema)
{
    size_t n = sizeof(keywords) / sizeof(keywords[0]);

    for (size_t i = 0; i < n; i++)
    {
        if (strcmp(lexema, keywords[i].lexema) == 0)
            return keywords[i].tipo;
    }

    return TK_ERROR; // o TK_IDENTIFICADOR
}

// Instrucciones ::= Instruccion
//                 | Instruccion Instrucciones
//void parsePrograma(void) {
//    parseInstrucciones();
//    match(TK_EOF);
//}
//
// void parseInstrucciones(void)
// {
//    do
//    {
//        parseInstruccion();
//    }
//    while (iniciaInstruccion(lookahead.tipo));
// }

//Instrucciones ::= Instruccion+
void parsePrograma(void)
{
    do
    {
        parseInstruccion();
    }
    while (iniciaInstruccion(lookahead.tipo));

    match(TK_EOF);
}

void parseInstruccion(void) {
    switch (lookahead.tipo) {
        case TK_WHEN:
            parseBloqueWhen();
            break;

        case TK_EVERY:
            parseBloqueEvery();
            break;

        case TK_IF:
            parseBloqueCondicional();
            break;

        case TK_FOCO_ID:
        case TK_AIRE_ID:
        case TK_PERSIANA_ID:
        case TK_CERRADURA_ID:
        case TK_RELOJ_ID:
        case TK_ALTAVOZ_ID:
        case TK_ALARMA_ID:
            parseAsignacion();
            break;

        default:
            errorSintactico("Se esperaba una instruccion");
    }
}

bool iniciaInstruccion(TokenType t)
{
    switch(t)
    {
        case TK_WHEN:
        case TK_EVERY:
        case TK_IF:
        case TK_FOCO_ID:
        case TK_AIRE_ID:
        case TK_PERSIANA_ID:
        case TK_CERRADURA_ID:
        case TK_RELOJ_ID:
        case TK_ALTAVOZ_ID:
        case TK_ALARMA_ID:
            return true;

        default:
            return false;
    }
}



void parseBloqueWhen(void)        { /* TODO issue #9 */ }
void parseBloqueEvery(void)       { /* TODO issue #9 */ }
void parseBloqueCondicional(void) { /* TODO issue #9 */ }
void parseAsignacion(void)        { /* TODO issue #9 */ }


// Faltaría implementar el reconocimiento de:
// 
// º palabras reservadas (WHEN, DO, IF, etc.),
// º identificadores de dispositivos (FOCO_patio, AIRE_comedor, etc.),
// º operadores (==, !=, >=, <=, =),
// º literales (23.5C, 80%, "texto", fechas, horas, emails, colores, modos).

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr,
                "Uso: %s archivo.dsl\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    if (!abrirFuente(argv[1]))
    {
        fprintf(stderr,
                "No se pudo abrir el archivo\n");
        return EXIT_FAILURE;
    }

    siguienteToken();

    parsePrograma();

    cerrarFuente();

    return EXIT_SUCCESS;
}