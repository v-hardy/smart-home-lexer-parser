#include <stdio.h>
#include <stdbool.h>


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
    int booleano;
} ValorToken;

typedef struct {
    TokenType tipo;
    char lexema[64];

    ValorToken valor;

    int linea;
    int columna;
} Token;


// <======================================= Variables globales =======================================>

Token lookahead;


// <======================================= Funciones =======================================>

// Con retorno vacio

// Instrucciones ::= Instruccion
//                 | Instruccion Instrucciones
//void parsePrograma(void) {
//    parseInstrucciones();
//    match(TK_EOF);
//}
//
// void parseInstrucciones(void)
// {
//     parseInstruccion();
//
//     while(iniciaInstruccion(lookahead.tipo))
//     {
//         parseInstruccion();
//     }
// }

//Instrucciones ::= Instruccion+
void parsePrograma(void) {
    parseInstrucciones();
    {
        do {
            parseInstruccion();
        } while (iniciaInstruccion(lookahead.tipo));
    }
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
        case TK_ALTAVOZ_ID:
        case TK_ALARMA_ID:
            parseAsignacion();
            break;

        default:
            errorSintactico("Se esperaba una instruccion");
    }
}

void match(TokenType esperado) {
    if (lookahead.tipo == esperado) {
        siguienteToken();
    } else {
        errorSintactico("Token inesperado");
    }
}

// Con retorno booleano
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
        case TK_ALTAVOZ_ID:
        case TK_ALARMA_ID:
            return true;

        default:
            return false;
    }
}