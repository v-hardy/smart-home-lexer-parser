/*
   Tests del lexer — Fase 1: keywords, operadores, booleanos, sensores y EOF.

   Compilar y correr:
       clang -Wall -std=c11 -DTESTING test_lexer.c -o test_lexer
       ./test_lexer

   El #include trae todo main.c en modo TESTING: el main() de main.c queda
   excluido (#ifndef TESTING) y la lectura de caracteres usa un buffer en
   memoria via lexerInitDesdeString(), sin tocar disco.
*/

#ifndef TESTING
#define TESTING   // por si se compila sin el flag -DTESTING
#endif
#include "main.c"


static int testsPasados  = 0;
static int testsFallados = 0;

static void assertToken(int linea, Token tok, TokenType tipoEsp, const char *lexemaEsp)
{
    if (tok.tipo == tipoEsp && strcmp(tok.lexema, lexemaEsp) == 0)
    {
        testsPasados++;
    }
    else
    {
        fprintf(stderr,
                "[FAIL] linea %d — esperaba tipo %d \"%s\", obtuvo tipo %d \"%s\"\n",
                linea, tipoEsp, lexemaEsp, tok.tipo, tok.lexema);
        testsFallados++;
    }
}

#define ASSERT_TOKEN(tipo, lexema) assertToken(__LINE__, obtenerSiguienteToken(), tipo, lexema)

static void assertTokenTexto(int linea, Token tok, TokenType tipoEsp,
                              const char *lexemaEsp, const char *textoEsp)
{
    if (tok.tipo == tipoEsp &&
        strcmp(tok.lexema, lexemaEsp) == 0 &&
        strcmp(tok.valor.texto, textoEsp) == 0)
    {
        testsPasados++;
    }
    else
    {
        fprintf(stderr,
                "[FAIL] linea %d — esperaba tipo %d \"%s\" texto=\"%s\", "
                "obtuvo tipo %d \"%s\" texto=\"%s\"\n",
                linea, tipoEsp, lexemaEsp, textoEsp,
                tok.tipo, tok.lexema, tok.valor.texto);
        testsFallados++;
    }
}

#define ASSERT_TOKEN_TEXTO(tipo, lexema, texto) \
    assertTokenTexto(__LINE__, obtenerSiguienteToken(), tipo, lexema, texto)


/* Todas las palabras reservadas */
void test_fase1_keywords(void)
{
    lexerInitDesdeString("WHEN DO END IF THEN ELSE EVERY AND OR NOT");

    ASSERT_TOKEN(TK_WHEN,  "WHEN");
    ASSERT_TOKEN(TK_DO,    "DO");
    ASSERT_TOKEN(TK_END,   "END");
    ASSERT_TOKEN(TK_IF,    "IF");
    ASSERT_TOKEN(TK_THEN,  "THEN");
    ASSERT_TOKEN(TK_ELSE,  "ELSE");
    ASSERT_TOKEN(TK_EVERY, "EVERY");
    ASSERT_TOKEN(TK_AND,   "AND");
    ASSERT_TOKEN(TK_OR,    "OR");
    ASSERT_TOKEN(TK_NOT,   "NOT");
    ASSERT_TOKEN(TK_EOF,   "");
}

/* Operadores simples y dobles, parentesis y delimitador.
   El delimitador del lenguaje es "." (no ";"). */
void test_fase1_operadores(void)
{
    lexerInitDesdeString("> >= < <= == != = ( ) .");

    ASSERT_TOKEN(TK_MAYOR,      ">");
    ASSERT_TOKEN(TK_MAYORIGUAL, ">=");
    ASSERT_TOKEN(TK_MENOR,      "<");
    ASSERT_TOKEN(TK_MENORIGUAL, "<=");
    ASSERT_TOKEN(TK_IGUAL,      "==");
    ASSERT_TOKEN(TK_DIFERENTE,  "!=");
    ASSERT_TOKEN(TK_ASIGNACION, "=");
    ASSERT_TOKEN(TK_PAR_IZQ,    "(");
    ASSERT_TOKEN(TK_PAR_DER,    ")");
    ASSERT_TOKEN(TK_DELIMITADOR, ".");
    ASSERT_TOKEN(TK_EOF,        "");
}

/* Literales booleanos: TRUE/FALSE (sensor) y ON/OFF (actuador) */
void test_fase1_booleanos(void)
{
    lexerInitDesdeString("TRUE FALSE ON OFF");

    ASSERT_TOKEN(TK_BOOL_SENSOR,   "TRUE");
    ASSERT_TOKEN(TK_BOOL_SENSOR,   "FALSE");
    ASSERT_TOKEN(TK_BOOL_ACTUADOR, "ON");
    ASSERT_TOKEN(TK_BOOL_ACTUADOR, "OFF");
    ASSERT_TOKEN(TK_EOF,           "");
}

/* Sensores definidos en keywords[] */
void test_fase1_sensores(void)
{
    lexerInitDesdeString("SENSOR_TEMP SENSOR_HUMEDAD SENSOR_LUZ SENSOR_MOVIMIENTO SENSOR_HUMO");

    ASSERT_TOKEN(TK_SENSOR_TEMP,       "SENSOR_TEMP");
    ASSERT_TOKEN(TK_SENSOR_HUMEDAD,    "SENSOR_HUMEDAD");
    ASSERT_TOKEN(TK_SENSOR_LUZ,        "SENSOR_LUZ");
    ASSERT_TOKEN(TK_SENSOR_MOVIMIENTO, "SENSOR_MOVIMIENTO");
    ASSERT_TOKEN(TK_SENSOR_HUMO,       "SENSOR_HUMO");
    ASSERT_TOKEN(TK_EOF,               "");
}

/* Input vacio y solo espacios retornan TK_EOF */
void test_fase1_eof(void)
{
    lexerInitDesdeString("");
    ASSERT_TOKEN(TK_EOF, "");

    lexerInitDesdeString("   \n\t  ");
    ASSERT_TOKEN(TK_EOF, "");
}

/* Fase 2: Literales numericos con unidad — temperatura */
void test_fase2_temp(void)
{
    lexerInitDesdeString("30\xC2\xB0""C 0\xC2\xB0""C");
    ASSERT_TOKEN(TK_TEMP, "30\xC2\xB0""C");
    ASSERT_TOKEN(TK_TEMP, "0\xC2\xB0""C");
    ASSERT_TOKEN(TK_EOF,  "");

    // Sin decimales: '.' es delimitador, no separador decimal
    lexerInitDesdeString("22.5\xC2\xB0""C");
    ASSERT_TOKEN(TK_ERROR, "22");
}

/* Fase 2: Literales numericos con unidad — porcentaje */
void test_fase2_porcentaje(void)
{
    lexerInitDesdeString("80% 0% 100%");
    ASSERT_TOKEN(TK_PORCENTAJE, "80%");
    ASSERT_TOKEN(TK_PORCENTAJE, "0%");
    ASSERT_TOKEN(TK_PORCENTAJE, "100%");
    ASSERT_TOKEN(TK_EOF, "");
}

/* Fase 2: Literales numericos con unidad — tiempo */
void test_fase2_tiempo(void)
{
    lexerInitDesdeString("2h 30min");
    ASSERT_TOKEN(TK_TIEMPO, "2h");
    ASSERT_TOKEN(TK_TIEMPO, "30min");
    ASSERT_TOKEN(TK_EOF, "");
}

/* Fase 2: Literales numericos con unidad — lux */
void test_fase2_lux(void)
{
    lexerInitDesdeString("500lux");
    ASSERT_TOKEN(TK_LUX, "500lux");
    ASSERT_TOKEN(TK_EOF, "");
}

/* Fase 2: Numero sin unidad es TK_ERROR */
void test_fase2_sin_unidad(void)
{
    lexerInitDesdeString("42");
    ASSERT_TOKEN(TK_ERROR, "42");
}

/* Fase 3: strings entre comillas */
void test_fase3_strings(void)
{
    lexerInitDesdeString("\"Hola mundo\" \"luz encendida\"");
    ASSERT_TOKEN_TEXTO(TK_TEXTO, "\"Hola mundo\"",    "Hola mundo");
    ASSERT_TOKEN_TEXTO(TK_TEXTO, "\"luz encendida\"", "luz encendida");
    ASSERT_TOKEN(TK_EOF, "");

    lexerInitDesdeString("\"sin cerrar");
    ASSERT_TOKEN(TK_ERROR, "\"sin cerrar");
}

/* Fase 3: emails */
void test_fase3_emails(void)
{
    lexerInitDesdeString("usuario@dominio.com admin@casa.org");
    ASSERT_TOKEN_TEXTO(TK_EMAIL, "usuario@dominio.com", "usuario@dominio.com");
    ASSERT_TOKEN_TEXTO(TK_EMAIL, "admin@casa.org",      "admin@casa.org");
    ASSERT_TOKEN(TK_EOF, "");
}

/* Fase 3: horas en formato HH:MM */
void test_fase3_horas(void)
{
    lexerInitDesdeString("08:30 14:00 23:59");
    ASSERT_TOKEN_TEXTO(TK_HORA, "08:30", "08:30");
    ASSERT_TOKEN_TEXTO(TK_HORA, "14:00", "14:00");
    ASSERT_TOKEN_TEXTO(TK_HORA, "23:59", "23:59");
    ASSERT_TOKEN(TK_EOF, "");
}

/* Fase 3: fechas en formato DD/MM/AAAA */
void test_fase3_fechas(void)
{
    lexerInitDesdeString("12/06/2024 01/01/2025");
    ASSERT_TOKEN_TEXTO(TK_FECHA, "12/06/2024", "12/06/2024");
    ASSERT_TOKEN_TEXTO(TK_FECHA, "01/01/2025", "01/01/2025");
    ASSERT_TOKEN(TK_EOF, "");
}

/* Fase 3: instruccion completa con multiples tipos de token */
void test_fase3_mixto(void)
{
    lexerInitDesdeString("WHEN SENSOR_TEMP > 30\xC2\xB0""C DO");
    ASSERT_TOKEN(TK_WHEN,        "WHEN");
    ASSERT_TOKEN(TK_SENSOR_TEMP, "SENSOR_TEMP");
    ASSERT_TOKEN(TK_MAYOR,       ">");
    ASSERT_TOKEN(TK_TEMP,        "30\xC2\xB0""C");
    ASSERT_TOKEN(TK_DO,          "DO");
    ASSERT_TOKEN(TK_EOF,         "");
}

/* Fase 3: comentarios de una linea son ignorados */
void test_fase3_comentarios(void)
{
    lexerInitDesdeString("WHEN // esto es un comentario\nDO");
    ASSERT_TOKEN(TK_WHEN, "WHEN");
    ASSERT_TOKEN(TK_DO,   "DO");
    ASSERT_TOKEN(TK_EOF,  "");
}

/* Fase 4: TK_MODO — FRIO, CALOR, VENT */
void test_fase4_modo(void)
{
    lexerInitDesdeString("FRIO CALOR VENT");
    ASSERT_TOKEN(TK_MODO, "FRIO");
    ASSERT_TOKEN(TK_MODO, "CALOR");
    ASSERT_TOKEN(TK_MODO, "VENT");
    ASSERT_TOKEN(TK_EOF,  "");

    lexerInitDesdeString("frio calor vent");
    ASSERT_TOKEN(TK_MODO, "frio");
    ASSERT_TOKEN(TK_MODO, "calor");
    ASSERT_TOKEN(TK_MODO, "vent");
    ASSERT_TOKEN(TK_EOF,  "");
}

/* Fase 4: TK_COLOR — blanco, rojo, azul, blue */
void test_fase4_color(void)
{
    lexerInitDesdeString("blanco rojo azul blue");
    ASSERT_TOKEN(TK_COLOR, "blanco");
    ASSERT_TOKEN(TK_COLOR, "rojo");
    ASSERT_TOKEN(TK_COLOR, "azul");
    ASSERT_TOKEN(TK_COLOR, "blue");
    ASSERT_TOKEN(TK_EOF,   "");

    lexerInitDesdeString("BLANCO ROJO AZUL BLUE");
    ASSERT_TOKEN(TK_COLOR, "BLANCO");
    ASSERT_TOKEN(TK_COLOR, "ROJO");
    ASSERT_TOKEN(TK_COLOR, "AZUL");
    ASSERT_TOKEN(TK_COLOR, "BLUE");
    ASSERT_TOKEN(TK_EOF,   "");
}

/* Fase 4: TK_TIEMPO — unidades m (minutos) y s (segundos) */
void test_fase4_tiempo_unidades(void)
{
    lexerInitDesdeString("30m 10s 5h");
    ASSERT_TOKEN(TK_TIEMPO, "30m");
    ASSERT_TOKEN(TK_TIEMPO, "10s");
    ASSERT_TOKEN(TK_TIEMPO, "5h");
    ASSERT_TOKEN(TK_EOF,    "");
}

/* Fase 4: case-insensitive — keywords y sensores */
void test_fase4_case_insensitive_keywords(void)
{
    lexerInitDesdeString("when do end if then else every and or not");
    ASSERT_TOKEN(TK_WHEN,  "when");
    ASSERT_TOKEN(TK_DO,    "do");
    ASSERT_TOKEN(TK_END,   "end");
    ASSERT_TOKEN(TK_IF,    "if");
    ASSERT_TOKEN(TK_THEN,  "then");
    ASSERT_TOKEN(TK_ELSE,  "else");
    ASSERT_TOKEN(TK_EVERY, "every");
    ASSERT_TOKEN(TK_AND,   "and");
    ASSERT_TOKEN(TK_OR,    "or");
    ASSERT_TOKEN(TK_NOT,   "not");
    ASSERT_TOKEN(TK_EOF,   "");
}

void test_fase4_case_insensitive_sensores(void)
{
    lexerInitDesdeString("sensor_temp sensor_humedad sensor_luz sensor_movimiento sensor_humo");
    ASSERT_TOKEN(TK_SENSOR_TEMP,       "sensor_temp");
    ASSERT_TOKEN(TK_SENSOR_HUMEDAD,    "sensor_humedad");
    ASSERT_TOKEN(TK_SENSOR_LUZ,        "sensor_luz");
    ASSERT_TOKEN(TK_SENSOR_MOVIMIENTO, "sensor_movimiento");
    ASSERT_TOKEN(TK_SENSOR_HUMO,       "sensor_humo");
    ASSERT_TOKEN(TK_EOF,               "");
}

void test_fase4_case_insensitive_booleanos(void)
{
    lexerInitDesdeString("true false on off");
    ASSERT_TOKEN(TK_BOOL_SENSOR,   "true");
    ASSERT_TOKEN(TK_BOOL_SENSOR,   "false");
    ASSERT_TOKEN(TK_BOOL_ACTUADOR, "on");
    ASSERT_TOKEN(TK_BOOL_ACTUADOR, "off");
    ASSERT_TOKEN(TK_EOF,           "");
}

void test_fase4_case_insensitive_dispositivos(void)
{
    lexerInitDesdeString("foco_sala aire_1 persiana_comedor");
    ASSERT_TOKEN(TK_FOCO_ID,     "foco_sala");
    ASSERT_TOKEN(TK_AIRE_ID,     "aire_1");
    ASSERT_TOKEN(TK_PERSIANA_ID, "persiana_comedor");
    ASSERT_TOKEN(TK_EOF,         "");
}

/* Fase 4: email con punto y guion antes del @ */
void test_fase4_email_complejo(void)
{
    lexerInitDesdeString("felipe@smart-home.com.ar bomberos@smart-home.com.ar");
    ASSERT_TOKEN_TEXTO(TK_EMAIL, "felipe@smart-home.com.ar",   "felipe@smart-home.com.ar");
    ASSERT_TOKEN_TEXTO(TK_EMAIL, "bomberos@smart-home.com.ar", "bomberos@smart-home.com.ar");
    ASSERT_TOKEN(TK_EOF, "");

    lexerInitDesdeString("user.name@dominio.com");
    ASSERT_TOKEN_TEXTO(TK_EMAIL, "user.name@dominio.com", "user.name@dominio.com");
    ASSERT_TOKEN(TK_EOF, "");
}

int main(void)
{
    test_fase1_keywords();
    test_fase1_operadores();
    test_fase1_booleanos();
    test_fase1_sensores();
    test_fase1_eof();

    test_fase2_temp();
    test_fase2_porcentaje();
    test_fase2_tiempo();
    test_fase2_lux();
    test_fase2_sin_unidad();

    test_fase3_strings();
    test_fase3_emails();
    test_fase3_horas();
    test_fase3_fechas();
    test_fase3_mixto();
    test_fase3_comentarios();

    test_fase4_modo();
    test_fase4_color();
    test_fase4_tiempo_unidades();
    test_fase4_case_insensitive_keywords();
    test_fase4_case_insensitive_sensores();
    test_fase4_case_insensitive_booleanos();
    test_fase4_case_insensitive_dispositivos();
    test_fase4_email_complejo();

    printf("\nResultados: %d pasaron, %d fallaron.\n", testsPasados, testsFallados);

    return testsFallados > 0 ? 1 : 0;
}
