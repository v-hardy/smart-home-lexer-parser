# Guía — Lexer/Parser Smart Home DSL

Referencia del proyecto para entender el código, las decisiones de diseño y los conceptos de C que aparecen en el camino.

---

## Tabla de contenidos

1. [¿Qué hace este proyecto?](#qué-hace-este-proyecto)
2. [Estructura de archivos](#estructura-de-archivos)
3. [Qué es un lexer y qué es un parser](#qué-es-un-lexer-y-qué-es-un-parser)
4. [Conceptos de C que vas a encontrar](#conceptos-de-c-que-vas-a-encontrar)
5. [Flujo completo: de texto a tokens](#flujo-completo-de-texto-a-tokens)
6. [Los tokens — representación de datos](#los-tokens--representación-de-datos)
7. [Explicación función por función](#explicación-función-por-función)
8. [Cómo funciona el modo de testing](#cómo-funciona-el-modo-de-testing)
9. [El Makefile](#el-makefile)
10. [Decisiones de diseño](#decisiones-de-diseño)
11. [Casos borde](#casos-borde)
12. [El parser — estado actual](#el-parser--estado-actual)
13. [Referencia rápida](#referencia-rápida)

---

## ¿Qué hace este proyecto?

Implementa un **lexer** y el comienzo de un **parser** para un lenguaje de automatización de hogar inteligente (DSL). El DSL permite escribir reglas como:

```
WHEN SENSOR_TEMP > 30°C DO
    FOCO_salon . BRILLO = 80%
END
```

El lexer convierte ese texto en tokens (unidades con significado). El parser verifica que la secuencia de tokens respete la gramática del lenguaje. El lexer está completo; el parser tiene la estructura pero sus funciones principales son stubs vacíos (issue #9).

---

## Estructura de archivos

```
smart-home-lexer-parser/
├── main.c          ← todo el código: lexer + stubs del parser
├── test_lexer.c    ← suite de tests
├── Makefile        ← instrucciones de compilación
├── lexer           ← binario compilado (generado por `make`)
└── test_lexer      ← binario de tests (generado por `make test`)
```

Todo el código del lexer y parser vive en `main.c`. No hay archivos `.h` separados. La razón es práctica: `test_lexer.c` hace `#include "main.c"` para incluir todo el código del lexer sin tener que compilar y linkear múltiples archivos. Para un proyecto de este tamaño, funciona bien.

---

## Qué es un lexer y qué es un parser

El **lexer** lee el texto carácter por carácter y lo agrupa en tokens. Un token es la unidad mínima con significado del lenguaje. Por ejemplo:

- `30°C` → un token de tipo `TK_TEMP` con valor numérico `30`
- `WHEN` → un token de tipo `TK_WHEN`
- `.` → un token de tipo `TK_DELIMITADOR`

El lexer no valida si la secuencia tiene sentido — solo identifica piezas individuales.

El **parser** recibe esa secuencia y verifica que siga las reglas de la gramática. Si después de `WHEN` no viene una condición válida, o falta el `DO`, el parser reporta error de sintaxis.

---

## Conceptos de C que vas a encontrar

### Includes

```c
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
```

En C la biblioteca estándar no se incluye automáticamente. Cada header habilita un grupo de funciones:

| Header | Funciones usadas |
|--------|-----------------|
| `stdio.h` | `fopen`, `fclose`, `fgetc`, `ungetc`, `fprintf`, `printf` |
| `stdbool.h` | `bool`, `true`, `false` |
| `stdlib.h` | `exit`, `EXIT_FAILURE`, `EXIT_SUCCESS`, `atof` |
| `ctype.h` | `isalpha`, `isdigit`, `isalnum`, `isspace` |
| `string.h` | `strcmp`, `strcasecmp`, `strncasecmp`, `strlen`, `strcpy`, `strncpy`, `strchr` |

Las funciones de `ctype.h` esperan `unsigned char`. Por eso en el código siempre se castea: `isspace((unsigned char)c)`. Si pasás un `char` con valor negativo directamente, el comportamiento es indefinido en algunas plataformas.

### #define — constantes y macros

```c
#define MAX_LEXEMA     64
#define MAX_LEXEMA_IDX (MAX_LEXEMA - 1)
```

`#define` es una directiva del preprocesador: antes de compilar, reemplaza textualmente cada aparición de `MAX_LEXEMA` por `64`. No es una variable, no ocupa memoria en tiempo de ejecución.

`MAX_LEXEMA_IDX` existe porque los strings en C terminan con `'\0'`. Si el array tiene 64 posiciones, la posición 63 siempre queda reservada para el terminador. `MAX_LEXEMA_IDX` es el último índice en que se puede escribir un carácter real:

```c
if (i < MAX_LEXEMA_IDX)    // hay espacio para escribir
    tk.lexema[i++] = ch;
```

### Compilación condicional — #ifdef / #ifndef

```c
#ifdef TESTING
    // se compila solo cuando se define TESTING
#else
    // se compila en modo normal
#endif
```

Permite tener dos versiones del binario desde el mismo código. Con `make` se compila el lexer normal (lee de archivo, tiene su propio `main`). Con `make test` se compila con `-DTESTING` y `test_lexer.c` aporta su propio `main`.

El `main()` de `main.c` está envuelto en `#ifndef TESTING` para que no existan dos funciones `main` cuando se compila para tests — eso sería un error de linkeo.

### typedef enum

```c
typedef enum {
    TK_ERROR,
    TK_WHEN,
    TK_DO,
    // ...
    TK_EOF
} TokenType;
```

Una `enum` es una lista de constantes enteras con nombre. El compilador les asigna valores automáticamente desde 0. `typedef` crea un alias para no tener que escribir `enum TokenType` en cada uso.

### typedef struct

```c
typedef struct {
    TokenType tipo;
    char lexema[MAX_LEXEMA];
    ValorToken valor;
    int linea;
    int columna;
} Token;
```

Un `struct` agrupa campos relacionados en un solo tipo. Cada token tiene el tipo que es, el texto original que apareció en el input, un valor semántico, y la posición en el archivo fuente.

### typedef union

```c
typedef union {
    double numero;
    int    booleano;
    char   texto[MAX_LEXEMA];
} ValorToken;
```

Una `union` es como un `struct` con una diferencia: todos sus campos comparten la misma memoria. El tamaño es el del campo más grande. Se usa porque un token nunca tiene valor numérico *y* valor de texto al mismo tiempo — uno de los dos siempre está sin usar. La `union` evita reservar memoria para ambos.

Para saber qué campo leer, siempre se consulta `token.tipo` primero. Si es `TK_TEMP`, se lee `token.valor.numero`. Si es `TK_HORA`, se lee `token.valor.texto`. Leer el campo equivocado da basura.

### Arrays y strings en C

En C no existe un tipo `string`. Un string es un array de `char` que termina con el carácter nulo `'\0'`. Las funciones de `string.h` leen hasta encontrar ese terminador — si no está, siguen leyendo más allá del array.

El patrón de llenado que vas a ver en todo el código:

```c
int i = 0;
while (condicion)
{
    if (i < MAX_LEXEMA_IDX)
        tk.lexema[i++] = ch;
    avanzarCaracter();
}
tk.lexema[i] = '\0';
```

Siempre se verifica que hay espacio antes de escribir, y siempre se termina con `'\0'`. Si el lexema fuera más largo que 63 caracteres, el código deja de copiarlo pero no desborda.

### Punteros a char — `const char *`

```c
typedef struct {
    const char *lexema;
    TokenType tipo;
} Keyword;
```

`const char *` es un puntero a un string que no se puede modificar. Los strings literales del código (`"WHEN"`, `"DO"`) viven en una sección de solo lectura del binario. En la tabla `keywords[]`, `lexema` apunta a esos literales. En el struct `Token`, en cambio, `lexema` es un array `char[64]` con su propia memoria que sí se puede escribir.

### Variables globales y estado del lexer

```c
Token lookahead;
static FILE *fuente;
static int caracterActual;
static int lineaActual = 1;
static int columnaActual = 0;
```

El lexer necesita recordar en qué carácter está, en qué línea y columna, y desde qué archivo lee. Por eso estas variables son globales: múltiples funciones las comparten. La palabra `static` limita su visibilidad al archivo `main.c`.

`lookahead` es el mecanismo del parser: siempre tiene un token leído por adelantado. El parser mira `lookahead.tipo` para decidir qué producción de la gramática aplicar y llama a `match()` para consumirlo.

### sizeof para contar elementos de un array

```c
size_t n = sizeof(keywords) / sizeof(keywords[0]);
```

`sizeof(keywords)` es el tamaño total del array en bytes. `sizeof(keywords[0])` es el tamaño de un elemento. La división da la cantidad de elementos. Si agregás o quitás entradas del array, el número se actualiza solo en tiempo de compilación.

### Funciones que retornan structs

```c
Token leerNumeroConUnidad(Token tk)
{
    // modifica tk
    return tk;
}
```

En C los structs se pasan por valor — se copia todo el struct. Esta función recibe una copia de `tk` (que ya tiene `linea` y `columna` cargados desde el llamador), la llena, y retorna la copia modificada. La alternativa sería pasar un puntero `Token *tk`, pero para structs de este tamaño el enfoque por valor es más simple de leer.

---

## Flujo completo: de texto a tokens

```
archivo / string de test
        │
        ▼
   leerChar()          ← lee un byte (archivo o buffer de test)
        │
        ▼
  caracterActual       ← el carácter "bajo la lupa" en este momento
        │
        ▼
  omitirEspacios()     ← salta espacios y comentarios //
        │
        ▼
  obtenerSiguienteToken()
        │
        ├── letra / '_'   → palabra (keyword, dispositivo, email)
        ├── dígito         → número con unidad
        ├── '"'            → string literal
        ├── operador       → =, ==, >, >=, etc.
        └── otro           → TK_ERROR
        │
        ▼
      Token              ← struct con tipo, lexema, valor, linea, columna
        │
        ▼
    lookahead            ← el parser lo consume con match()
```

---

## Los tokens — representación de datos

Cada token tiene tres piezas de información que importan:

**`tipo`** — qué es el token (uno de los valores del enum `TokenType`):

```
TK_ERROR              carácter no reconocido, o número sin unidad
TK_WHEN/DO/END/...    palabras reservadas
TK_AND/OR/NOT         operadores lógicos
TK_IGUAL/MAYOR/...    operadores de comparación
TK_ASIGNACION         =
TK_DELIMITADOR        .
TK_SENSOR_*           SENSOR_TEMP, SENSOR_HUMEDAD, etc.
TK_FOCO_ID/...        identificadores de dispositivos
TK_ATRIB_*            BRILLO, ESTADO, COLOR, etc.
TK_BOOL_SENSOR        TRUE, FALSE
TK_BOOL_ACTUADOR      ON, OFF
TK_TEMP               30°C       → valor.numero = 30
TK_PORCENTAJE         80%        → valor.numero = 80
TK_TIEMPO             2h, 30min  → valor.numero = 2
TK_LUX                500lux     → valor.numero = 500
TK_HORA               08:30      → valor.texto = "08:30"
TK_FECHA              12/06/2024 → valor.texto = "12/06/2024"
TK_TEXTO              "mensaje"  → valor.texto = "mensaje"
TK_EMAIL              u@dom.com  → valor.texto = "u@dom.com"
TK_MODO               FRIO, CALOR, VENT
TK_COLOR              BLANCO, ROJO, AZUL
TK_EOF                fin del archivo
```

**`lexema`** — el texto original tal como apareció en el input. Para `TK_TEXTO` incluye las comillas: `"Hola mundo"`.

**`valor`** — el valor semántico extraído. Para `TK_TEXTO`, `valor.texto` tiene el contenido sin comillas: `Hola mundo`. Para tokens numéricos, `valor.numero` tiene el número. Para `TK_EMAIL` y `TK_HORA`, `lexema` y `valor.texto` son iguales.

---

## Explicación función por función

### `abrirFuente` / `cerrarFuente`

```c
int abrirFuente(const char *nombreArchivo)
{
    fuente = fopen(nombreArchivo, "r");
    if (fuente == NULL) return 0;
    caracterActual = leerChar();
    return 1;
}
```

Además de abrir el archivo, lee el primer carácter. El lexer siempre trabaja con un carácter "pre-leído" en `caracterActual` — nunca pide el siguiente sin haber procesado el actual. Retorna `1` si el archivo se abrió, `0` si no existe.

### `avanzarCaracter`

```c
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
```

Actualiza la posición (línea/columna) y carga el siguiente carácter. Todos los mensajes de error del lexer usan estas variables para indicar dónde está el problema.

### `obtenerSiguienteCaracter` — peek sin consumir

```c
int obtenerSiguienteCaracter(void)
{
#ifdef TESTING
    if (_testBuffer[_testPos] == '\0') return EOF;
    return (unsigned char)_testBuffer[_testPos];
#else
    int c = fgetc(fuente);
    if (c != EOF) ungetc(c, fuente);
    return c;
#endif
}
```

Mira el siguiente carácter sin avanzar. Se usa para distinguir `>` de `>=`, `=` de `==`, `!` de `!=`, y para detectar `//` de comentario.

En modo normal: `fgetc` lee el carácter y `ungetc` lo "devuelve" al stream. En modo test: mira directamente en el buffer sin mover el índice.

### `omitirEspacios`

Salta espacios, tabs, saltos de línea, y comentarios de línea (`// ...`). Siempre se llama al inicio de `obtenerSiguienteToken`.

Una decisión no obvia: cuando termina de consumir el `//`, el `'\n'` que cierra la línea no se consume dentro del bloque de comentario. Se deja para que el `isspace` de la próxima iteración lo procese y llame a `avanzarCaracter()`, que es quien actualiza `lineaActual`. Si se consumiera el `'\n'` dentro del bloque de comentario, habría que actualizar `lineaActual` también ahí — duplicación de lógica.

### `obtenerSiguienteToken` — el corazón del lexer

Esta es la función más larga. Su estructura es simple: llama a `omitirEspacios`, mira el primer carácter, y despacha al manejador correspondiente.

El caso más complejo es cuando el carácter es una letra o `_`, porque puede ser:
- Keyword: `WHEN`, `SENSOR_TEMP`, `ESTADO`
- Dispositivo dinámico: `FOCO_sala`, `AIRE_comedor`
- Email: `usuario@dominio.com`
- Booleano, modo, color

La estrategia: leer todos los caracteres que puedan pertenecer al token (incluyendo `@`, `-`, `+`, y `.` si hay un `@` adelante), y después consultar primero `buscarKeyword()` y si no, `reconocerDispositivo()`.

### `leerNumeroConUnidad`

Lee la parte entera y luego mira qué carácter sigue para determinar el tipo:

| Sigue | Resultado |
|-------|-----------|
| `:` | intenta leer `HH:MM` → `TK_HORA` o `TK_ERROR` |
| `/` | intenta leer `DD/MM/AAAA` → `TK_FECHA` o `TK_ERROR` |
| `%` | `TK_PORCENTAJE` |
| `0xC2` | primer byte de `°` en UTF-8 → intenta leer `°C` → `TK_TEMP` |
| letra | lee la unidad: `h`/`m`/`min`/`s` → `TK_TIEMPO`, `lux` → `TK_LUX` |
| otro | `TK_ERROR` — número sin unidad no es válido en este DSL |

El símbolo `°` no existe en ASCII. En UTF-8 se codifica con dos bytes: `0xC2 0xB0`. El código los detecta secuencialmente: primero `0xC2`, luego verifica `0xB0`, luego `'C'`. Si algo no coincide, `TK_ERROR`.

### `leerTexto`

Lee un string entre comillas dobles. El `lexema` incluye las comillas (`"Hola mundo"`); `valor.texto` tiene el contenido sin ellas (`Hola mundo`). Si el archivo termina antes de encontrar la comilla de cierre, retorna `TK_ERROR`.

### `buscarKeyword` / `reconocerDispositivo`

`buscarKeyword` hace búsqueda lineal en el array `keywords[]` usando `strcasecmp` (case-insensitive). Si no encuentra el lexema, retorna `TK_ERROR` — señal para que el llamador pruebe con `reconocerDispositivo`.

`reconocerDispositivo` compara solo el prefijo usando `strncasecmp`. La condición `strlen(lexema) > len` asegura que haya sufijo después del prefijo. `FOCO_` solo, sin nada después, es `TK_ERROR`.

### `hayArrobaAdelante`

Resuelve la ambigüedad del punto en emails. El problema: `.` es el delimitador del lenguaje, pero también aparece dentro de emails como `user.name@dominio.com`.

Cuando el lexer está leyendo una palabra que empieza con letra, la condición del loop es:

```c
obtenerCaracterActual() == '.' && hayArrobaAdelante()
```

Solo incluye el `.` en el token si hay un `@` más adelante en la línea (antes de un espacio). Así `user.name@dominio.com` se lee entero, pero en `FOCO_sala . BRILLO` el punto detiene la lectura del identificador.

### `match` y `siguienteToken`

```c
void match(TokenType esperado)
{
    if (lookahead.tipo == esperado)
        siguienteToken();
    else
        errorSintactico("Token inesperado");
}
```

El parser es predictivo descendente: mira `lookahead` para decidir qué producción aplicar, luego consume tokens con `match`. Si el token no es el esperado, `errorSintactico` imprime la posición y termina el programa.

---

## Cómo funciona el modo de testing

`test_lexer.c` define `TESTING` y hace `#include "main.c"`. Eso trae todo el código del lexer al mismo archivo de compilación. El `main()` de `main.c` queda excluido por `#ifndef TESTING`. `test_lexer.c` aporta su propio `main()`.

En modo test, `leerChar()` lee de un buffer en memoria en vez de un archivo:

```c
#ifdef TESTING
static char _testBuffer[4096];
static int  _testPos;

static int leerChar(void)
{
    if (_testBuffer[_testPos] == '\0') return EOF;
    return (unsigned char)_testBuffer[_testPos++];
}

void lexerInitDesdeString(const char *texto)
{
    strncpy(_testBuffer, texto, sizeof(_testBuffer) - 1);
    _testBuffer[sizeof(_testBuffer) - 1] = '\0';
    _testPos       = 0;
    lineaActual    = 1;
    columnaActual  = 0;
    caracterActual = leerChar();
}
#endif
```

`lexerInitDesdeString` reinicia todo el estado del lexer. Cada función de test la llama con su input y arranca desde cero.

Las macros de assert usan `__LINE__` para capturar el número de línea en tiempo de compilación:

```c
#define ASSERT_TOKEN(tipo, lexema) assertToken(__LINE__, obtenerSiguienteToken(), tipo, lexema)
```

Cuando un test falla, el mensaje indica exactamente en qué línea de `test_lexer.c` está el assert que falló. Los tests se escriben en secuencia: cada `ASSERT_TOKEN` consume un token del input y verifica tipo y lexema.

---

## El Makefile

```bash
make          # compila main.c → binario 'lexer'
make test     # compila con -DTESTING → binario 'test_lexer', lo ejecuta
make clean    # borra los binarios
```

El compilador se detecta automáticamente (gcc si está, sino clang). `-Wall` activa todos los warnings. `-std=c11` especifica el estándar C11.

`make test` compila `test_lexer.c` (que incluye `main.c` internamente) con `-DTESTING`. Si algún test falla, `test_lexer` retorna exit code 1 y `make` reporta el error.

---

## Decisiones de diseño

**El `.` es delimitador, no separador decimal.** El DSL usa `FOCO_sala . BRILLO = 80%` — el punto separa dispositivo de atributo. Por eso los números son solo enteros. `22.5°C` genera `TK_ERROR("22")` + `TK_DELIMITADOR(".")` + `TK_TEMP("5°C")`. Si en el futuro se necesitaran decimales, el separador sería `,`.

**Los dispositivos son dinámicos (por prefijo).** El lexer no tiene una lista de `FOCO_salon`, `FOCO_comedor`, etc. Reconoce cualquier token que empiece con `FOCO_` seguido de al menos un carácter. Esto permite múltiples dispositivos del mismo tipo sin tocar el lexer. Por eso los dispositivos están en `dispositivos[]` separados de `keywords[]`.

**`TRUE`/`FALSE` vs `ON`/`OFF`.** Son dos tokens distintos aunque ambos sean booleanos. `TK_BOOL_SENSOR` se usa para sensores (temperatura verdadera/falsa), `TK_BOOL_ACTUADOR` para actuadores (encendido/apagado). El parser puede exigir uno u otro según el atributo.

**Case-insensitive.** `buscarKeyword` y `reconocerDispositivo` usan `strcasecmp` / `strncasecmp`. El lenguaje acepta `WHEN`, `when` y `When` como equivalentes.

---

## Casos borde

**UTF-8 y el símbolo °.** `°` se codifica con dos bytes en UTF-8: `0xC2 0xB0`. Al copiarlo al lexema, la guarda es `i < MAX_LEXEMA_IDX - 1` (no `MAX_LEXEMA_IDX`) para que queden dos posiciones libres — una para `0xB0` y otra para `'C'`.

**`obtenerSiguienteCaracter()` en modo normal.** Usa `fgetc` + `ungetc`. Si `fuente == NULL` (el archivo no está abierto), esto crashea. En modo test no hay riesgo porque lee del buffer en memoria, que siempre existe.

**Prefijo de dispositivo sin sufijo.** `FOCO_` sin nada después es `TK_ERROR`. Lo garantiza la condición `strlen(lexema) > len` en `reconocerDispositivo`.

**String sin cerrar.** `"Hola mundo` (sin comilla de cierre) retorna `TK_ERROR` con el lexema parcial.

---

## El parser — estado actual

El parser usa descenso recursivo: una función por producción de la gramática.

```c
void parsePrograma(void)          // Instrucciones ::= Instruccion+
void parseInstruccion(void)       // dispatcher según el token inicial
void parseBloqueWhen(void)        // WHEN condición DO ... END
void parseBloqueEvery(void)       // EVERY intervalo DO ... END
void parseBloqueCondicional(void) // IF condición THEN ... ELSE ... END
void parseAsignacion(void)        // dispositivo . atributo = valor
```

`parsePrograma` y `parseInstruccion` están implementados: reconocen qué instrucción viene mirando `lookahead` y delegan. `iniciaInstruccion()` devuelve `true` si el token puede iniciar una instrucción válida.

Los cuatro restantes (`parseBloqueWhen`, `parseBloqueEvery`, `parseBloqueCondicional`, `parseAsignacion`) son stubs vacíos — retornan sin hacer nada. Su implementación está definida en el issue #9, que tiene la gramática completa. Leerlo antes de arrancar con cualquiera de ellos.

---

## Referencia rápida

| Función | Descripción |
|---------|-------------|
| `abrirFuente(archivo)` | Abre el archivo y carga el primer carácter |
| `cerrarFuente()` | Cierra el archivo |
| `avanzarCaracter()` | Avanza un carácter, actualiza línea/columna |
| `obtenerCaracterActual()` | Devuelve el carácter actual |
| `obtenerSiguienteCaracter()` | Peek del siguiente sin avanzar |
| `omitirEspacios()` | Salta espacios y comentarios `//` |
| `finDeArchivo()` | `true` si llegamos al EOF |
| `obtenerSiguienteToken()` | Devuelve el próximo token completo |
| `leerNumeroConUnidad(tk)` | Lee `30°C`, `80%`, `2h`, `08:30`, `12/06/2024`, etc. |
| `leerTexto(tk)` | Lee `"string entre comillas"` |
| `buscarKeyword(lexema)` | Busca en la tabla de keywords (case-insensitive) |
| `reconocerDispositivo(lexema)` | Reconoce `FOCO_x`, `AIRE_x`, etc. por prefijo |
| `hayArrobaAdelante()` | Lookahead para resolver `.` en emails (solo modo TESTING) |
| `siguienteToken()` | Carga el próximo token en `lookahead` |
| `match(tipo)` | Consume `lookahead` si coincide, error si no |
| `nombreToken(tipo)` | Nombre legible del tipo (para debug) |
| `volcarTokens()` | Imprime todos los tokens del archivo (flag `--tokens`) |
| `lexerInitDesdeString(texto)` | Reinicia el lexer con un string (solo modo TESTING) |

```bash
make                          # compilar
make test                     # correr tests
make clean                    # borrar binarios
./lexer archivo.dsl           # procesar un archivo DSL
./lexer archivo.dsl --tokens  # imprimir todos los tokens (debug)
```
