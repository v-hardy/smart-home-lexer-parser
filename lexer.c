#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"

// lexer.c
//  ↓
// lexer.h
//  ↓
// token.h

// <======================================= Prototipos internos =======================================>

static int finDeArchivo(void);
static void avanzarCaracter(void);
static void omitirEspacios(void);


// <======================================= Variables Globales =======================================>

Token lookahead;

static FILE *fuente = NULL;

static int caracterActual;
static int lineaActual = 1;
static int columnaActual = 0;


// <======================================= Palabras Reservadas =======================================>

typedef struct {
    const char *lexema;
    TokenType tipo;
} Keyword;

static Keyword keywords[] = {
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

static PrefijoDispositivo dispositivos[] = {
    {"FOCO_",      TK_FOCO_ID},
    {"AIRE_",      TK_AIRE_ID},
    {"PERSIANA_",  TK_PERSIANA_ID},
    {"CERRADURA_", TK_CERRADURA_ID},
    {"RELOJ_",     TK_RELOJ_ID},
    {"ALTAVOZ_",   TK_ALTAVOZ_ID},
    {"ALARMA_",    TK_ALARMA_ID},
};


// <======================================= Funciones privadas =======================================>

/* Lectura de caracteres */
/*
   Toda la lectura de caracteres pasa por leerChar(). En modo normal lee del
   archivo; en modo TESTING lee de un buffer en memoria, para poder testear
   el lexer desde un string sin tocar disco (ver lexerInitDesdeString).
*/
#ifdef TESTING

static char _testBuffer[4096];
static int  _testPos;

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

/* Avanzar un caracter */
static void avanzarCaracter(void)
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
static int obtenerCaracterActual(void)
{
    return caracterActual;
}

/* Peek del siguiente caracter sin consumirlo. Usa leerChar + ungetChar
   para funcionar tanto en modo normal como en TESTING. */
static int obtenerSiguienteCaracter(void)
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
static void omitirEspacios(void)
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
static int finDeArchivo(void)
{
    return caracterActual == EOF;
}

/* Creador token EOF */
static Token crearTokenEOF(void)
{
    Token tk;

    tk.tipo = TK_EOF;
    tk.lexema[0] = '\0';
    tk.linea = lineaActual;
    tk.columna = columnaActual;

    return tk;
}

static Token leerTexto(Token tk)
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
static Token leerNumeroConUnidad(Token tk)
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

        if (strcmp(unidad, "h") == 0 || strcmp(unidad, "min") == 0)
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

/* Buscar en palabras reservadas */
static TokenType buscarKeyword(const char *lexema)
{
    size_t n = sizeof(keywords) / sizeof(keywords[0]);

    for (size_t i = 0; i < n; i++)
    {
        if (strcmp(lexema, keywords[i].lexema) == 0)
            return keywords[i].tipo;
    }

    return TK_ERROR; // o TK_IDENTIFICADOR
}

/* Reconocer dispositivo dinamico por prefijo (FOCO_, AIRE_, ...) */
static TokenType reconocerDispositivo(const char *lexema)
{
    size_t n = sizeof(dispositivos) / sizeof(dispositivos[0]);

    for (size_t i = 0; i < n; i++)
    {
        size_t len = strlen(dispositivos[i].prefijo);

        // Coincide el prefijo Y hay al menos un caracter de sufijo
        if (strncmp(lexema, dispositivos[i].prefijo, len) == 0 &&
            strlen(lexema) > len)
        {
            return dispositivos[i].tipo;
        }
    }

    return TK_ERROR;
}

/* Nombre legible de un TokenType (para modo debug --tokens) */
static const char *nombreToken(TokenType t)
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


// <======================================= Funciones publicas =======================================>

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
                obtenerCaracterActual() == '-'))
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
        {
            t = reconocerDispositivo(tk.lexema);

            /* colores y modos */
            if (t == TK_ERROR)
            {
                if (strcmp(tk.lexema, "BLANCO") == 0 ||
                    strcmp(tk.lexema, "ROJO") == 0 ||
                    strcmp(tk.lexema, "AZUL") == 0)
                {
                    t = TK_COLOR;
                }
                else if (strcmp(tk.lexema, "FRIO") == 0 ||
                        strcmp(tk.lexema, "CALOR") == 0 ||
                        strcmp(tk.lexema, "VENT") == 0)
                {
                    t = TK_MODO;
                }
            }

            /* bools */
            if (t == TK_ERROR)
            {
                if (strcmp(tk.lexema, "TRUE") == 0 ||
                    strcmp(tk.lexema, "FALSE") == 0)
                {
                    t = TK_BOOL_SENSOR;
                }
                else if (strcmp(tk.lexema, "ON") == 0 ||
                        strcmp(tk.lexema, "OFF") == 0)
                {
                    t = TK_BOOL_ACTUADOR;
                }
            }
        }

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

/* Coincidencia del siguiente con lo esperado */
void match(TokenType esperado) 
// {
//     if (lookahead.tipo == esperado) {
//         siguienteToken();
//     } else {
//         errorSintactico("Token inesperado");
//     }
// }
{
    if (lookahead.tipo == esperado)
    {
        printf("MATCH OK: %s\n", lookahead.lexema);
        siguienteToken();
    }
    else
    {
        printf("MATCH FAIL\n");
        printf("esperado: %d\n", esperado);
        printf("recibido: %d (%s)\n",
               lookahead.tipo,
               lookahead.lexema);

        errorSintactico("Token inesperado");
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
