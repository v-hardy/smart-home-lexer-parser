// <======================================= Definiciones de tipo de datos =======================================>

#ifndef TOKEN_H
#define TOKEN_H
typedef enum {
    TK_ERROR = 0,

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

#endif