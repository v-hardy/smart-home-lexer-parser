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
    {"EVERY", TK_EVERY},
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

    /* Valores discretos de modo aire */
    {"FRIO",  TK_MODO},
    {"CALOR", TK_MODO},
    {"VENT",  TK_MODO},

    /* Valores de color */
    {"BLANCO", TK_COLOR},
    {"ROJO",   TK_COLOR},
    {"AZUL",   TK_COLOR},

};


// <======================================= Identificadores de dispositivos =======================================>

/*
   Los dispositivos son identificadores DINAMICOS (decision de diseño, issue #2):
   se reconocen por PREFIJO + sufijo. Ej: FOCO_sala, AIRE_comedor, FOCO_1.
   El sufijo acepta caracteres alfanumericos y '_'. Por eso NO van en keywords[].
*/
typedef struct {
    const char *prefijo;
    TokenType   tipo;
} PrefijoDispositivo;

PrefijoDispositivo dispositivos[] = {
    {"FOCO_",      TK_FOCO_ID},
    {"AIRE_",      TK_AIRE_ID},
    {"PERSIANA_",  TK_PERSIANA_ID},
    {"CERRADURA_", TK_CERRADURA_ID},
    {"RELOJ_",     TK_RELOJ_ID},
    {"ALTAVOZ_",   TK_ALTAVOZ_ID},
    {"ALARMA_",    TK_ALARMA_ID},
};


// <======================================= Literales y Unidades (Tokens Compuestos) =======================================>

/* Tamaño máximo de lexema y valor.texto. MAX_LEXEMA_IDX es el último índice
   escribible — el byte [MAX_LEXEMA - 1] queda siempre reservado para '\0'. */
#define MAX_LEXEMA     64
#define MAX_LEXEMA_IDX (MAX_LEXEMA - 1)

typedef union {
    double numero;
    int    booleano;
    char   texto[MAX_LEXEMA];
} ValorToken;

typedef struct {
    TokenType tipo;
    char lexema[MAX_LEXEMA];

    ValorToken valor;

    int linea;
    int columna;
} Token;


// <======================================= Prototipos =======================================>

int        finDeArchivo(void);
TokenType  buscarKeyword(const char *lexema);
TokenType  reconocerDispositivo(const char *lexema);
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


// <======================================= Lectura de caracteres =======================================>

/*
   Toda la lectura de caracteres pasa por leerChar(). En modo normal lee del
   archivo; en modo TESTING lee de un buffer en memoria, para poder testear
   el lexer desde un string sin tocar disco (ver lexerInitDesdeString).
*/
#ifdef TESTING

static char _testBuffer[4096];
static int  _testPos;

/* Escanea adelante en el buffer sin consumir. Retorna 1 si hay '@' antes de espacio/EOF. */
static int hayArrobaAdelante(void)
{
    for (int i = _testPos; _testBuffer[i] != '\0'; i++)
    {
        if (_testBuffer[i] == '@') return 1;
        if (isspace((unsigned char)_testBuffer[i])) return 0;
    }
    return 0;
}

#else

static int hayArrobaAdelante(void) { return 0; }

#endif  /* cierre del bloque TESTING/else para hayArrobaAdelante */

#ifdef TESTING  /* reabre para leerChar y lexerInitDesdeString */

static int leerChar(void)
{
    if (_testBuffer[_testPos] == '\0')
        return EOF;

    return (unsigned char)_testBuffer[_testPos++];
}

/* Reinicia el lexer para leer desde un string (solo modo test) */
void lexerInitDesdeString(const char *texto)
{
    strncpy(_testBuffer, texto, sizeof(_testBuffer) - 1);
    _testBuffer[sizeof(_testBuffer) - 1] = '\0';

    _testPos       = 0;
    lineaActual    = 1;
    columnaActual  = 0;
    caracterActual = leerChar();
}

#else

static int leerChar(void)
{
    return fgetc(fuente);
}

#endif


// <======================================= Funciones =======================================>

//* Abrir archivo */
int abrirFuente(const char *nombreArchivo)
{
    fuente = fopen(nombreArchivo, "r");

    if (fuente == NULL)
        return 0;

    caracterActual = leerChar();

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

    caracterActual = leerChar();
}

/* Obtiene caracter actual */
int obtenerCaracterActual(void)
{
    return caracterActual;
}

/* Peek del siguiente caracter sin consumirlo. Usa leerChar + ungetChar
   para funcionar tanto en modo normal como en TESTING. */
int obtenerSiguienteCaracter(void)
{
#ifdef TESTING
    if (_testBuffer[_testPos] == '\0')
        return EOF;
    return (unsigned char)_testBuffer[_testPos];
#else
    int c = fgetc(fuente);
    if (c != EOF)
        ungetc(c, fuente);
    return c;
#endif
}

/* Omitir espacios y comentarios */
void omitirEspacios(void)
{
    while (!finDeArchivo())
    {
        /* Espacios, tabs, saltos de línea, etc. */
        if (isspace(caracterActual))
        {
            avanzarCaracter();
            continue;
        }

        /* Comentario de una línea */
        if (caracterActual == '/' &&
            obtenerSiguienteCaracter() == '/')
        {
            /* Avanzar "//" */
            avanzarCaracter();
            avanzarCaracter();

            /* Avanzar hasta fin de línea */
            while (!finDeArchivo() &&
                   caracterActual != '\n')
            {
                avanzarCaracter();
            }

            /* El '\n' será consumido en la próxima iteración
               por el bloque isspace(), actualizando lineaActual */
            continue;
        }

        /* Ya no hay espacios ni comentarios */
        break;
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

Token leerTexto(Token tk)
{
    int i = 0;
    int j = 0;

    /* Lexema incluye comillas: "Hola mundo". valor.texto es el contenido sin comillas. */
    tk.lexema[i++] = '"';
    avanzarCaracter();

    while (!finDeArchivo() &&
           obtenerCaracterActual() != '"')
    {
        int ch = obtenerCaracterActual();
        if (i < MAX_LEXEMA_IDX)     tk.lexema[i++] = (char)ch;
        if (j < MAX_LEXEMA_IDX)     tk.valor.texto[j++] = (char)ch;
        avanzarCaracter();
    }

    if (obtenerCaracterActual() != '"')
    {
        tk.lexema[i] = '\0';
        tk.valor.texto[j] = '\0';
        tk.tipo = TK_ERROR;
        return tk;
    }

    if (i < MAX_LEXEMA_IDX) tk.lexema[i++] = '"';
    tk.lexema[i] = '\0';
    tk.valor.texto[j] = '\0';
    avanzarCaracter();

    tk.tipo = TK_TEXTO;
    return tk;
}


/*
   Literal numerico con unidad: 30°C, 80%, 2h, 30min, 500lux.
   Solo enteros (decision de diseño, issue #5): el TP maneja valores enteros y
   el '.' es delimitador, no separador decimal. Un numero sin unidad valida es
   TK_ERROR. Recibe el token con linea/columna ya cargadas.
*/
Token leerNumeroConUnidad(Token tk)
{
    int i = 0;

    // Parte entera
    while (!finDeArchivo() && isdigit(obtenerCaracterActual()))
    {
        if (i < MAX_LEXEMA_IDX)
            tk.lexema[i++] = (char)obtenerCaracterActual();
        avanzarCaracter();
    }

    /* Hora HH:MM */
    if (obtenerCaracterActual() == ':')
    {
        if (i < MAX_LEXEMA_IDX)
            tk.lexema[i++] = ':';

        avanzarCaracter();

        while (!finDeArchivo() &&
               isdigit(obtenerCaracterActual()))
        {
            if (i < MAX_LEXEMA_IDX)
                tk.lexema[i++] = (char)obtenerCaracterActual();

            avanzarCaracter();
        }

        tk.lexema[i] = '\0';

        if (strlen(tk.lexema) == 5 &&
            isdigit(tk.lexema[0]) &&
            isdigit(tk.lexema[1]) &&
            tk.lexema[2] == ':' &&
            isdigit(tk.lexema[3]) &&
            isdigit(tk.lexema[4]))
        {
            int hh = (tk.lexema[0] - '0') * 10 +
                     (tk.lexema[1] - '0');

            int mm = (tk.lexema[3] - '0') * 10 +
                     (tk.lexema[4] - '0');

            if (hh >= 0 && hh <= 23 &&
                mm >= 0 && mm <= 59)
            {
                strncpy(tk.valor.texto, tk.lexema, MAX_LEXEMA_IDX);
                tk.valor.texto[MAX_LEXEMA_IDX] = '\0';
                tk.tipo = TK_HORA;
                return tk;
            }
        }

        tk.tipo = TK_ERROR;
        return tk;
    }

    /* Fecha DD/MM/AAAA */
    if (obtenerCaracterActual() == '/')
    {
        if (i < MAX_LEXEMA_IDX) tk.lexema[i++] = '/';
        avanzarCaracter();

        int digits = 0;
        while (!finDeArchivo() && isdigit(obtenerCaracterActual()) && digits < 2)
        {
            if (i < MAX_LEXEMA_IDX) tk.lexema[i++] = (char)obtenerCaracterActual();
            avanzarCaracter();
            digits++;
        }

        if (digits != 2 || obtenerCaracterActual() != '/')
        {
            tk.lexema[i] = '\0';
            tk.tipo = TK_ERROR;
            return tk;
        }

        if (i < MAX_LEXEMA_IDX) tk.lexema[i++] = '/';
        avanzarCaracter();

        digits = 0;
        while (!finDeArchivo() && isdigit(obtenerCaracterActual()) && digits < 4)
        {
            if (i < MAX_LEXEMA_IDX) tk.lexema[i++] = (char)obtenerCaracterActual();
            avanzarCaracter();
            digits++;
        }
        tk.lexema[i] = '\0';

        if (digits == 4)
        {
            strncpy(tk.valor.texto, tk.lexema, MAX_LEXEMA_IDX);
            tk.valor.texto[MAX_LEXEMA_IDX] = '\0';
            tk.tipo = TK_FECHA;
        }
        else
        {
            tk.tipo = TK_ERROR;
        }
        return tk;
    }

    tk.lexema[i] = '\0';
    tk.valor.numero = atof(tk.lexema);

    int u = obtenerCaracterActual();

    // Porcentaje: 80%
    if (u == '%')
    {
        if (i < MAX_LEXEMA_IDX)
            tk.lexema[i++] = '%';
        tk.lexema[i] = '\0';
        avanzarCaracter();
        tk.tipo = TK_PORCENTAJE;
        return tk;
    }

    // Temperatura: 30°C  ('°' es 0xC2 0xB0 en UTF-8, debe seguir 'C')
    if (u == 0xC2)
    {
        if (i < MAX_LEXEMA_IDX - 1)
            tk.lexema[i++] = (char)0xC2;
        avanzarCaracter();

        if (obtenerCaracterActual() == 0xB0)
        {
            if (i < MAX_LEXEMA_IDX - 1)
                tk.lexema[i++] = (char)0xB0;
            avanzarCaracter();

            if (obtenerCaracterActual() == 'C')
            {
                if (i < MAX_LEXEMA_IDX)
                    tk.lexema[i++] = 'C';
                tk.lexema[i] = '\0';
                avanzarCaracter();
                tk.tipo = TK_TEMP;
                return tk;
            }
        }

        tk.lexema[i] = '\0';
        tk.tipo = TK_ERROR;
        return tk;
    }

    // Unidades alfabeticas: h y min (tiempo), lux
    if (isalpha(u))
    {
        char unidad[8];
        int  j = 0;

        while (!finDeArchivo() && isalpha(obtenerCaracterActual()))
        {
            int ch = obtenerCaracterActual();

            if (j < (int)sizeof(unidad) - 1)
                unidad[j++] = (char)ch;
            if (i < MAX_LEXEMA_IDX)
                tk.lexema[i++] = (char)ch;

            avanzarCaracter();
        }
        unidad[j] = '\0';
        tk.lexema[i] = '\0';

        if (strcmp(unidad, "h") == 0 || strcmp(unidad, "m") == 0 ||
            strcmp(unidad, "min") == 0 || strcmp(unidad, "s") == 0)
            tk.tipo = TK_TIEMPO;
        else if (strcmp(unidad, "lux") == 0)
            tk.tipo = TK_LUX;
        else
            tk.tipo = TK_ERROR;

        return tk;
    }

    // Numero sin unidad -> invalido
    tk.lexema[i] = '\0';
    tk.tipo = TK_ERROR;
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
    tk.valor.numero = 0;   // fase 1: sin valor asociado todavia

    int c = obtenerCaracterActual();

    /* Palabras: keywords (WHEN, SENSOR_TEMP, ESTADO...) e identificadores
       dinamicos de dispositivo (FOCO_sala, AIRE_1...). Empiezan con letra o '_'. */
    if (isalpha(c) || c == '_')
    {
        int i = 0;
        while (!finDeArchivo() &&
            (isalnum(obtenerCaracterActual()) ||
                obtenerCaracterActual() == '_' ||
                obtenerCaracterActual() == '@' ||
                obtenerCaracterActual() == '-' ||
                obtenerCaracterActual() == '+' ||
                (obtenerCaracterActual() == '.' && hayArrobaAdelante())))
        {
            if (i < MAX_LEXEMA_IDX)
                tk.lexema[i++] = (char)obtenerCaracterActual();
            avanzarCaracter();
        }
        tk.lexema[i] = '\0';

        if (strchr(tk.lexema, '@') != NULL)
        {
            while (!finDeArchivo())
            {
                int ch = obtenerCaracterActual();
                if (isalnum(ch) || ch == '.' || ch == '-' || ch == '_' || ch == '+')
                {
                    if (i < MAX_LEXEMA_IDX)
                        tk.lexema[i++] = (char)ch;
                    avanzarCaracter();
                }
                else break;
            }
            tk.lexema[i] = '\0';
            strcpy(tk.valor.texto, tk.lexema);
            tk.tipo = TK_EMAIL;
            return tk;
        }

        // Primero palabra reservada fija; si no, prefijo de dispositivo dinamico
        TokenType t = buscarKeyword(tk.lexema);
        if (t == TK_ERROR)
            t = reconocerDispositivo(tk.lexema);

        tk.tipo = t;
        return tk;
    }

    /* Literales numericos con unidad o literal de hora: HH:MM*: 30°C, 80%, 2h, 30min, 500lux, HH:MM. */
    if (isdigit(c))
    {
        return leerNumeroConUnidad(tk);
    }

    /* Literal de texto */
    if (c == '"')
    {
        return leerTexto(tk);
    }

    /* Operadores, parentesis y delimitador.
       La posicion (linea/columna) ya quedo guardada arriba, antes de avanzar. */
    switch (c)
    {
        case '>':
            avanzarCaracter();
            if (obtenerCaracterActual() == '=')
            {
                avanzarCaracter();
                tk.tipo = TK_MAYORIGUAL;
                strcpy(tk.lexema, ">=");
            }
            else
            {
                tk.tipo = TK_MAYOR;
                strcpy(tk.lexema, ">");
            }
            return tk;

        case '<':
            avanzarCaracter();
            if (obtenerCaracterActual() == '=')
            {
                avanzarCaracter();
                tk.tipo = TK_MENORIGUAL;
                strcpy(tk.lexema, "<=");
            }
            else
            {
                tk.tipo = TK_MENOR;
                strcpy(tk.lexema, "<");
            }
            return tk;

        case '=':
            avanzarCaracter();
            if (obtenerCaracterActual() == '=')
            {
                avanzarCaracter();
                tk.tipo = TK_IGUAL;
                strcpy(tk.lexema, "==");
            }
            else
            {
                tk.tipo = TK_ASIGNACION;
                strcpy(tk.lexema, "=");
            }
            return tk;

        case '!':
            avanzarCaracter();
            if (obtenerCaracterActual() == '=')
            {
                avanzarCaracter();
                tk.tipo = TK_DIFERENTE;
                strcpy(tk.lexema, "!=");
            }
            else
            {
                tk.tipo = TK_ERROR;
                strcpy(tk.lexema, "!");
            }
            return tk;

        case '(':
            avanzarCaracter();
            tk.tipo = TK_PAR_IZQ;
            strcpy(tk.lexema, "(");
            return tk;

        case ')':
            avanzarCaracter();
            tk.tipo = TK_PAR_DER;
            strcpy(tk.lexema, ")");
            return tk;

        case '.':   // delimitador del lenguaje (decision de diseño, issue #3)
            avanzarCaracter();
            tk.tipo = TK_DELIMITADOR;
            strcpy(tk.lexema, ".");
            return tk;
    }

    /* Caracter no reconocido. Los literales numericos y de texto llegan en
       issues posteriores (#5, #7). Se consume para no trabar el lexer. */
    tk.lexema[0] = (char)c;
    tk.lexema[1] = '\0';
    tk.tipo = TK_ERROR;
    avanzarCaracter();

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

/* Buscar en palabras reservadas (case-insensitive) */
TokenType buscarKeyword(const char *lexema)
{
    size_t n = sizeof(keywords) / sizeof(keywords[0]);

    for (size_t i = 0; i < n; i++)
    {
        if (strcasecmp(lexema, keywords[i].lexema) == 0)
            return keywords[i].tipo;
    }

    return TK_ERROR;
}

/* Reconocer dispositivo dinamico por prefijo (case-insensitive) */
TokenType reconocerDispositivo(const char *lexema)
{
    size_t n = sizeof(dispositivos) / sizeof(dispositivos[0]);

    for (size_t i = 0; i < n; i++)
    {
        size_t len = strlen(dispositivos[i].prefijo);

        if (strncasecmp(lexema, dispositivos[i].prefijo, len) == 0 &&
            strlen(lexema) > len)
        {
            return dispositivos[i].tipo;
        }
    }

    return TK_ERROR;
}

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

/* Nombre legible de un TokenType (para modo debug --tokens) */
const char *nombreToken(TokenType t)
{
    switch (t)
    {
        case TK_ERROR:            return "TK_ERROR";
        case TK_WHEN:             return "TK_WHEN";
        case TK_DO:               return "TK_DO";
        case TK_END:              return "TK_END";
        case TK_EVERY:            return "TK_EVERY";
        case TK_IF:               return "TK_IF";
        case TK_THEN:             return "TK_THEN";
        case TK_ELSE:             return "TK_ELSE";
        case TK_PAR_IZQ:          return "TK_PAR_IZQ";
        case TK_PAR_DER:          return "TK_PAR_DER";
        case TK_AND:              return "TK_AND";
        case TK_OR:               return "TK_OR";
        case TK_NOT:              return "TK_NOT";
        case TK_IGUAL:            return "TK_IGUAL";
        case TK_DIFERENTE:        return "TK_DIFERENTE";
        case TK_MAYOR:            return "TK_MAYOR";
        case TK_MAYORIGUAL:       return "TK_MAYORIGUAL";
        case TK_MENOR:            return "TK_MENOR";
        case TK_MENORIGUAL:       return "TK_MENORIGUAL";
        case TK_ASIGNACION:       return "TK_ASIGNACION";
        case TK_DELIMITADOR:      return "TK_DELIMITADOR";
        case TK_SENSOR_TEMP:      return "TK_SENSOR_TEMP";
        case TK_SENSOR_HUMEDAD:   return "TK_SENSOR_HUMEDAD";
        case TK_SENSOR_LUZ:       return "TK_SENSOR_LUZ";
        case TK_SENSOR_MOVIMIENTO:return "TK_SENSOR_MOVIMIENTO";
        case TK_SENSOR_HUMO:      return "TK_SENSOR_HUMO";
        case TK_FOCO_ID:          return "TK_FOCO_ID";
        case TK_AIRE_ID:          return "TK_AIRE_ID";
        case TK_PERSIANA_ID:      return "TK_PERSIANA_ID";
        case TK_CERRADURA_ID:     return "TK_CERRADURA_ID";
        case TK_RELOJ_ID:         return "TK_RELOJ_ID";
        case TK_ALTAVOZ_ID:       return "TK_ALTAVOZ_ID";
        case TK_ALARMA_ID:        return "TK_ALARMA_ID";
        case TK_ATRIB_ESTADO:     return "TK_ATRIB_ESTADO";
        case TK_ATRIB_BRILLO:     return "TK_ATRIB_BRILLO";
        case TK_ATRIB_COLOR:      return "TK_ATRIB_COLOR";
        case TK_ATRIB_TEMP_O:     return "TK_ATRIB_TEMP_O";
        case TK_ATRIB_TEMP_A:     return "TK_ATRIB_TEMP_A";
        case TK_ATRIB_MODO:       return "TK_ATRIB_MODO";
        case TK_ATRIB_POSICION:   return "TK_ATRIB_POSICION";
        case TK_ATRIB_HORA:       return "TK_ATRIB_HORA";
        case TK_ATRIB_FECHA:      return "TK_ATRIB_FECHA";
        case TK_ATRIB_MUTE:       return "TK_ATRIB_MUTE";
        case TK_ATRIB_VOLUMEN:    return "TK_ATRIB_VOLUMEN";
        case TK_ATRIB_EMAIL_NOTIF:return "TK_ATRIB_EMAIL_NOTIF";
        case TK_ATRIB_MENSAJE:    return "TK_ATRIB_MENSAJE";
        case TK_ATRIB_ACTIVADA:   return "TK_ATRIB_ACTIVADA";
        case TK_BOOL_SENSOR:      return "TK_BOOL_SENSOR";
        case TK_BOOL_ACTUADOR:    return "TK_BOOL_ACTUADOR";
        case TK_TEMP:             return "TK_TEMP";
        case TK_PORCENTAJE:       return "TK_PORCENTAJE";
        case TK_TIEMPO:           return "TK_TIEMPO";
        case TK_LUX:              return "TK_LUX";
        case TK_HORA:             return "TK_HORA";
        case TK_FECHA:            return "TK_FECHA";
        case TK_TEXTO:            return "TK_TEXTO";
        case TK_EMAIL:            return "TK_EMAIL";
        case TK_MODO:             return "TK_MODO";
        case TK_COLOR:            return "TK_COLOR";
        case TK_EOF:              return "TK_EOF";
        default:                  return "TK_DESCONOCIDO";
    }
}

/* Modo debug: lexea el archivo e imprime cada token (no parsea) */
void volcarTokens(void)
{
    Token tk;
    do
    {
        tk = obtenerSiguienteToken();
        printf("[%d:%d] %-20s '%s'\n",
               tk.linea, tk.columna, nombreToken(tk.tipo), tk.lexema);
    }
    while (tk.tipo != TK_EOF);
}

/* En modo TESTING, test_lexer.c hace #include "main.c" y aporta su propio
   main(). Por eso aca se excluye este main() para evitar el conflicto. */
#ifndef TESTING

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr,
                "Uso: %s archivo.dsl [--tokens]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    if (!abrirFuente(argv[1]))
    {
        fprintf(stderr,
                "No se pudo abrir el archivo\n");
        return EXIT_FAILURE;
    }

    // Modo debug: volcar tokens del lexer sin parsear
    if (argc >= 3 && strcmp(argv[2], "--tokens") == 0)
    {
        volcarTokens();
        cerrarFuente();
        return EXIT_SUCCESS;
    }

    siguienteToken();

    parsePrograma();

    cerrarFuente();

    return EXIT_SUCCESS;
}

#endif  // TESTING
