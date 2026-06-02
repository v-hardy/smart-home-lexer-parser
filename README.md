# Guía del Proyecto: Lexer/Parser Smart Home

## ¿Qué es este proyecto?

Es un **analizador de lenguaje** para un lenguaje de dominio específico (DSL) de domótica (smart home). La idea es que el usuario pueda escribir reglas como esta:

```
WHEN SENSOR_TEMP > 30°C DO
    AIRE_1.ESTADO = ON
END
```

Y el programa entienda esa instrucción, la analice y determine si es válida.

---

## Conceptos previos (para entender sin saber C)

### ¿Qué es un Lexer?

Un **lexer** (también llamado tokenizador) toma texto crudo y lo convierte en "piezas" con nombre. Es el primer paso antes de entender el significado.

**Analogía:** Igual que cuando leés una oración:
> "El perro come hueso"

Tu cerebro primero identifica: `[artículo] [sustantivo] [verbo] [sustantivo]`. Eso es lo que hace el lexer — etiqueta cada pieza.

**Ejemplo con el código:**
```
WHEN SENSOR_TEMP > 30°C DO
```
El lexer produce:
```
[TK_WHEN]  [TK_SENSOR_TEMP]  [TK_MAYOR]  [TK_TEMP: 30°C]  [TK_DO]
```

Cada pieza se llama **Token**.

---

### ¿Qué es un Parser?

El **parser** toma la lista de tokens del lexer y verifica que están en el orden correcto según las reglas del lenguaje (la gramática). También construye el significado.

**Analogía:** Si el lexer identifica las palabras, el parser verifica que la oración tenga sentido gramaticalmente.

---

### Tipos de datos en C relevantes para este código

| Concepto C | Qué es | Equivalente aproximado |
|-----------|--------|----------------------|
| `enum`    | Lista de constantes numéricas con nombre | Como un tipo con valores fijos |
| `struct`  | Agrupación de variables relacionadas | Como un objeto/registro |
| `union`   | Variables que comparten la misma memoria | Una caja que guarda UNA cosa a la vez |
| `typedef` | Le da un nombre nuevo a un tipo | Como un alias |
| `static`  | Variable/función visible solo en su archivo | Como "privado" en otros lenguajes |
| `FILE*`   | Puntero a un archivo abierto | Como un cursor de lectura sobre un archivo |

---

## Estructura del código (`main.c`)

El archivo tiene estos bloques en orden:

```
main.c
├── 1. Includes
├── 2. TokenType (enum)          → Lista de todos los tipos de tokens posibles
├── 3. Keyword (struct/tabla)    → Tabla de palabras reservadas del lenguaje
├── 4. Token + ValorToken        → Cómo se representa un token individual
├── 5. Variables globales        → lookahead (parser) + estado del lexer (FILE*)
├── 6. Funciones del lexer       → leer archivo, avanzar, omitir espacios, etc.
└── 7. Funciones del parser      → parsePrograma, match, parseInstruccion, etc.
```

---

## Bloque 1: `TokenType` — Todos los tokens posibles

```c
typedef enum {
    TK_ERROR,
    TK_WHEN,
    TK_DO,
    ...
    TK_EOF
} TokenType;
```

Es una lista numerada de todos los "tipos de pieza" que puede producir el lexer. C los convierte internamente en números (0, 1, 2...) pero el código los usa por nombre para que sea legible.

### Categorías de tokens definidas:

| Categoría | Ejemplos | Para qué sirve |
|-----------|---------|----------------|
| **Palabras reservadas** | `WHEN`, `DO`, `END`, `IF`, `THEN`, `ELSE`, `EVERY` | Estructura del lenguaje |
| **Paréntesis** | `(`, `)` | Agrupar condiciones |
| **Operadores lógicos** | `AND`, `OR`, `NOT` | Combinar condiciones |
| **Operadores comparación** | `>`, `<`, `==`, `!=`, `>=`, `<=` | Comparar valores |
| **Asignación** | `=` | Asignar valor a dispositivo |
| **Delimitador** | `;` o similar | Separar instrucciones |
| **Sensores** | `SENSOR_TEMP`, `SENSOR_HUMEDAD`, etc. | Fuentes de datos |
| **Dispositivos** | `FOCO_ID`, `AIRE_ID`, etc. | Cosas que se controlan |
| **Atributos** | `ESTADO`, `BRILLO`, `COLOR`, etc. | Propiedades de dispositivos |
| **Literales** | `TRUE/FALSE`, `ON/OFF`, temperaturas, porcentajes | Valores concretos |
| **EOF** | Fin del archivo | Señal de que no hay más texto |

---

## Bloque 2: `Keyword` — Tabla de palabras reservadas

```c
typedef struct {
    const char *lexema;  // el texto tal cual ("WHEN", "IF", etc.)
    TokenType tipo;      // el tipo de token que representa
} Keyword;

Keyword keywords[] = {
    {"WHEN", TK_WHEN},
    {"DO",   TK_DO},
    ...
};
```

Es una tabla de búsqueda. El lexer lee una palabra del texto, la busca en esta tabla con `buscarKeyword()` y si la encuentra, sabe qué tipo de token es.

**Ejemplo:** Lee `"WHEN"` → busca en la tabla → encuentra `{"WHEN", TK_WHEN}` → produce token de tipo `TK_WHEN`.

---

## Bloque 3: `Token` — La estructura de un token individual

```c
typedef union {
    double numero;   // para valores como 30.5 (temperatura, porcentaje)
    int booleano;    // para TRUE/FALSE
} ValorToken;

typedef struct {
    TokenType tipo;    // qué tipo es (TK_WHEN, TK_TEMP, etc.)
    char lexema[64];   // el texto original ("WHEN", "30.5", etc.)
    ValorToken valor;  // el valor numérico/booleano si aplica
    int linea;         // en qué línea del código fuente está
    int columna;       // en qué columna está
} Token;
```

Cada token que produce el lexer tiene estos 5 campos. `linea` y `columna` sirven para dar mensajes de error precisos ("Error en línea 5, columna 12").

**Importante:** `ValorToken` es un `union`, no un `struct`. Eso significa que `numero` y `booleano` comparten la misma memoria — solo uno está activo a la vez. El código que usa el token debe saber cuál usar según el `tipo`.

---

## Bloque 4: Variables globales y estado del lexer

```c
Token lookahead;              // token actual que el parser está mirando

static FILE   *fuente;        // archivo .dsl abierto
static int     caracterActual;// carácter leído en este momento
static int     lineaActual;   // línea actual (para errores)
static int     columnaActual; // columna actual (para errores)
```

`lookahead` es el token que el parser tiene "a la vista". El parser siempre mira un token adelante para decidir qué regla aplicar — patrón llamado **análisis predictivo LL(1)**.

Las variables `static` del lexer son privadas al archivo. El lexer las actualiza a medida que avanza por el archivo fuente.

---

## Bloque 5: Funciones del lexer

### Ciclo de vida del archivo

```c
abrirFuente("programa.dsl")   // abre el archivo, lee el primer carácter
    ↓
avanzarCaracter()              // mueve al siguiente carácter, actualiza línea/columna
    ↓
cerrarFuente()                 // cierra el archivo al terminar
```

### Funciones de lectura

| Función | Qué hace |
|---------|---------|
| `abrirFuente(nombre)` | Abre el archivo `.dsl`, lee el primer carácter |
| `cerrarFuente()` | Cierra el archivo |
| `avanzarCaracter()` | Lee el siguiente carácter, actualiza `lineaActual`/`columnaActual` |
| `obtenerCaracterActual()` | Retorna el carácter que se está mirando ahora |
| `finDeArchivo()` | Retorna `1` si ya se llegó al final |
| `omitirEspacios()` | Avanza saltando espacios, tabs y saltos de línea |

### Funciones de tokenización

| Función | Qué hace |
|---------|---------|
| `buscarKeyword(texto)` | Busca `texto` en `keywords[]`, retorna su `TokenType` o `TK_ERROR` |
| `crearTokenEOF()` | Construye el token de fin de archivo |
| `obtenerSiguienteToken()` | **Stub — pendiente.** Debe producir el próximo token completo |
| `siguienteToken()` | Llama a `obtenerSiguienteToken()` y guarda resultado en `lookahead` |

---

## Bloque 6: Funciones del parser

### `parsePrograma()`
Punto de entrada del parser. Procesa instrucciones hasta que no quede ninguna más, luego verifica que el archivo terminó (`TK_EOF`).

### `parseInstruccion()`
Mira el token actual (`lookahead`) y decide qué tipo de instrucción es:
- Si empieza con `WHEN` → bloque de evento
- Si empieza con `EVERY` → bloque de tiempo
- Si empieza con `IF` → condicional
- Si empieza con un dispositivo → asignación

### `match(TokenType esperado)`
Verifica que el token actual sea el que se espera. Si sí, avanza al siguiente token. Si no, reporta error de sintaxis.

**Analogía:** Como un lector que espera encontrar una coma en cierta posición. Si no está, dice "¡Falta la coma!".

### `iniciaInstruccion(TokenType t)`
Pregunta: "¿este tipo de token puede iniciar una instrucción válida?". Retorna `true` o `false`. Se usa para saber cuándo dejar de parsear instrucciones.

---

## El lenguaje que se está construyendo

```
// Bloque WHEN: "cuando ocurra X, hacer Y"
WHEN <condicion> DO
    <instrucciones>
END

// Bloque EVERY: "cada cierto tiempo, hacer Y"
EVERY <tiempo> DO
    <instrucciones>
END

// Condicional
IF <condicion> THEN
    <instrucciones>
ELSE
    <instrucciones>
END

// Asignación de atributo de dispositivo
FOCO_1.BRILLO = 80%
AIRE_1.ESTADO = ON

// Condiciones con sensores
SENSOR_TEMP > 30°C
SENSOR_HUMEDAD == 60%
SENSOR_MOVIMIENTO == TRUE
```

---

## Estado actual del proyecto

### ✅ Implementado y correcto

| Qué | Dónde |
|-----|-------|
| Todos los tipos de token definidos | `TokenType` enum |
| Tabla de keywords (parcial) | `keywords[]` |
| Estructura del token | `Token`, `ValorToken` |
| Apertura/cierre de archivo | `abrirFuente()`, `cerrarFuente()` |
| Avance de carácter con tracking de posición | `avanzarCaracter()` |
| Skip de espacios | `omitirEspacios()` |
| Detección de fin de archivo | `finDeArchivo()` |
| Búsqueda en tabla de keywords | `buscarKeyword()` |
| Token EOF | `crearTokenEOF()` |
| Asignación de lookahead | `siguienteToken()` |
| Estructura del parser | `parsePrograma()`, `parseInstruccion()`, `match()`, `iniciaInstruccion()` |
| Punto de entrada con argumento de archivo | `main()` |

### ❌ Pendiente (no existe o es stub)

| Qué falta | Por qué importa |
|-----------|----------------|
| **`obtenerSiguienteToken()` — lógica real** | Es el corazón del lexer. Hoy es un stub vacío. Sin esto no se reconoce ningún token |
| `errorSintactico()` | Se llama en `match()` y `parseInstruccion()` — sin ella el programa no compila |
| `parseBloqueWhen()` | Lógica para `WHEN...DO...END` |
| `parseBloqueEvery()` | Lógica para `EVERY...DO...END` |
| `parseBloqueCondicional()` | Lógica para `IF...THEN...ELSE...END` |
| `parseAsignacion()` | Lógica para `DISPOSITIVO.ATRIBUTO = VALOR` |

---

## Bugs pendientes

### Bug 1 — Faltan 2 includes

`isspace()` requiere `<ctype.h>`. `strcmp()` requiere `<string.h>`. Sin ellos: error de compilación.

```c
// Agregar al inicio:
#include <ctype.h>
#include <string.h>
```

---

### Bug 2 — Faltan prototipos de funciones

C requiere declarar las funciones antes de usarlas. Varias se llaman antes de estar definidas. Solución: agregar un bloque de prototipos antes de las implementaciones:

```c
void  errorSintactico(const char *mensaje);
void  parseInstruccion(void);
bool  iniciaInstruccion(TokenType t);
void  parseBloqueWhen(void);
void  parseBloqueEvery(void);
void  parseBloqueCondicional(void);
void  parseAsignacion(void);
int   finDeArchivo(void);
```

---

### Bug 3 — `"EVERY"` falta en `keywords[]`

`TK_EVERY` existe en el enum pero `"EVERY"` no está en la tabla. El lexer nunca produciría ese token.

```c
{"EVERY", TK_EVERY},   // agregar en keywords[]
```

---

### Bug 4 — Dispositivos faltan en `keywords[]`

Los 7 dispositivos existen en el enum pero sin entrada en la tabla de keywords:

```c
{"FOCO_ID",      TK_FOCO_ID},
{"AIRE_ID",      TK_AIRE_ID},
{"PERSIANA_ID",  TK_PERSIANA_ID},
{"CERRADURA_ID", TK_CERRADURA_ID},
{"RELOJ_ID",     TK_RELOJ_ID},
{"ALTAVOZ_ID",   TK_ALTAVOZ_ID},
{"ALARMA_ID",    TK_ALARMA_ID},
```

> **Decisión pendiente del grupo:** ¿Los dispositivos son palabras reservadas fijas (`FOCO_ID`) o identificadores dinámicos (`FOCO_sala`, `FOCO_2`)? Si son dinámicos, el lexer debe reconocerlos por prefijo, no por tabla.

---

### Bug 5 — `TK_RELOJ_ID` sin `case` en los switches

Está en el enum pero `parseInstruccion()` e `iniciaInstruccion()` no lo manejan.

---

### Bug 6 — `ValorToken` no soporta texto

Los tokens `TK_TEXTO`, `TK_EMAIL`, `TK_FECHA`, `TK_HORA` necesitan guardar strings. El union actual solo tiene `double` e `int`:

```c
typedef union {
    double numero;
    int    booleano;
    char   texto[64];   // ← falta esto
} ValorToken;
```

---

## Cómo compilar, correr y testear

### 1. Compilar

Desde la raíz del proyecto (donde está `main.c`):

```bash
clang -Wall -std=c11 main.c -o lexer
```

- `-Wall` activa todos los warnings (ayuda a cazar errores temprano).
- `-std=c11` fija el estándar de C.
- `-o lexer` genera un ejecutable llamado `lexer`.

Si no tenés `clang`, `gcc` funciona igual:

```bash
gcc -Wall -std=c11 main.c -o lexer
```

**Resultado esperado:** ningún mensaje y se crea el archivo `lexer`. Si aparecen errores/warnings, hay que corregirlos antes de seguir.

### 2. Crear un archivo de prueba

El programa recibe **un** archivo `.dsl` como argumento. Creá uno:

```bash
cat > programa.dsl <<'EOF'
WHEN SENSOR_TEMP > 30 DO
    AIRE_1.ESTADO = ON
END
EOF
```

### 3. Correr

```bash
./lexer programa.dsl
```

### 4. Qué esperar HOY (estado actual)

El lexer todavía es un stub (`obtenerSiguienteToken()` no reconoce tokens). Por eso, los casos que ya se pueden testear son los de **manejo de argumentos y archivos**, no el análisis real:

| Caso | Comando | Salida esperada | Código de salida |
|------|---------|-----------------|------------------|
| Sin argumentos | `./lexer` | `Uso: ./lexer archivo.dsl` | `1` |
| Archivo inexistente | `./lexer no_existe.dsl` | `No se pudo abrir el archivo` | `1` |
| Archivo válido | `./lexer programa.dsl` | `Error sintáctico [línea 1, col 0]: Se esperaba una instruccion` | `1` |

> El último error es **esperado**: el parser recibe un token vacío porque el lexer aún no produce tokens reales. Cuando se implemente `obtenerSiguienteToken()`, ese caso deberá pasar sin error.

Para ver el código de salida después de correr:

```bash
./lexer programa.dsl; echo "Exit: $?"
```

`Exit: 0` = todo bien. Cualquier otro número = error.

### 5. Cómo sabremos que el lexer funciona (objetivo)

Una vez implementado `obtenerSiguienteToken()`, el flujo de testeo será:

1. Escribir un `.dsl` con sintaxis **válida** → debe salir con código `0` y sin errores.
2. Escribir un `.dsl` con sintaxis **inválida** (ej: `WHEN DO END` sin condición) → debe imprimir un error sintáctico con línea/columna correcta.
3. (Opcional, recomendado) Agregar una opción de "modo debug" que imprima cada token reconocido, para verificar visualmente que el lexer separa bien las piezas.

---

## Próximos pasos (en orden)

1. Agregar `#include <ctype.h>` y `#include <string.h>`
2. Agregar prototipos de funciones faltantes
3. Implementar `errorSintactico()` — mínimo: imprimir línea/columna y salir
4. Agregar stubs vacíos para `parseBloqueWhen/Every/Condicional/Asignacion()` — desbloquea compilación
5. Completar `keywords[]` con `EVERY` y dispositivos
6. Implementar la lógica real de `obtenerSiguienteToken()` — el lexer de verdad
7. Implementar `parseBloqueWhen()` y el resto de funciones de parse
