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
| **Delimitador** | `.` | Separar instrucciones y `DISPOSITIVO.ATRIBUTO` |
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
| `obtenerSiguienteToken()` | Reconoce **palabras** (keywords fijas y dispositivos dinámicos), **operadores** (`>`, `<`, `=`, `==`, `!=`, `>=`, `<=`), paréntesis y delimitador `.`. Literales y números aún pendientes |
| `reconocerDispositivo(texto)` | Reconoce dispositivos dinámicos por prefijo (`FOCO_`, `AIRE_`...), retorna su `TokenType` o `TK_ERROR` |
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
| Tabla de keywords completa (reservadas, sensores, atributos, `EVERY`) | `keywords[]` |
| Reconocimiento de dispositivos dinámicos por prefijo | `dispositivos[]`, `reconocerDispositivo()` |
| Estructura del token (incluye `texto[64]` para strings) | `Token`, `ValorToken` |
| Apertura/cierre de archivo | `abrirFuente()`, `cerrarFuente()` |
| Avance de carácter con tracking de posición | `avanzarCaracter()` |
| Skip de espacios | `omitirEspacios()` |
| Detección de fin de archivo | `finDeArchivo()` |
| Búsqueda en tabla de keywords | `buscarKeyword()` |
| Lexeo de palabras (keywords + dispositivos) | `obtenerSiguienteToken()` |
| Lexeo de operadores, paréntesis y delimitador `.` | `obtenerSiguienteToken()` |
| Token EOF | `crearTokenEOF()` |
| Reporte de error con posición | `errorSintactico()` |
| Modo debug que vuelca tokens | `--tokens`, `volcarTokens()`, `nombreToken()` |
| Estructura del parser | `parsePrograma()`, `parseInstruccion()`, `match()`, `iniciaInstruccion()` |
| Punto de entrada con argumento de archivo | `main()` |

### ❌ Pendiente (no existe o es stub)

| Qué falta | Por qué importa |
|-----------|----------------|
| **Lexeo de literales y números** (`30°C`, `80%`, `"texto"`, fechas, horas, emails) | Sin esto no se pueden expresar valores concretos; hoy los dígitos caen en `TK_ERROR` |
| `parseBloqueWhen()` | Lógica para `WHEN...DO...END` (hoy stub vacío) |
| `parseBloqueEvery()` | Lógica para `EVERY...DO...END` (hoy stub vacío) |
| `parseBloqueCondicional()` | Lógica para `IF...THEN...ELSE...END` (hoy stub vacío) |
| `parseAsignacion()` | Lógica para `DISPOSITIVO.ATRIBUTO = VALOR` (hoy stub vacío) |

> Mientras falte el lexeo de operadores/literales, el **parseo normal no es funcional** (los `parseBloque*` son stubs). Para verificar el lexer usar el modo `--tokens`.

---

## Decisiones de diseño

### Dispositivos = identificadores dinámicos (no palabras fijas)

Los dispositivos **no** son palabras reservadas fijas. Se reconocen por **prefijo + sufijo**:

| Prefijo | Token | Ejemplos válidos |
|---------|-------|------------------|
| `FOCO_` | `TK_FOCO_ID` | `FOCO_sala`, `FOCO_1`, `FOCO_cocina` |
| `AIRE_` | `TK_AIRE_ID` | `AIRE_comedor`, `AIRE_1` |
| `PERSIANA_` | `TK_PERSIANA_ID` | `PERSIANA_living` |
| `CERRADURA_` | `TK_CERRADURA_ID` | `CERRADURA_entrada` |
| `RELOJ_` | `TK_RELOJ_ID` | `RELOJ_principal` |
| `ALTAVOZ_` | `TK_ALTAVOZ_ID` | `ALTAVOZ_cocina` |
| `ALARMA_` | `TK_ALARMA_ID` | `ALARMA_casa` |

- El **sufijo** acepta caracteres alfanuméricos y `_`, y debe tener al menos un carácter (`FOCO_` solo, sin sufijo, **no** es un dispositivo válido).
- Por ser dinámicos, los dispositivos **no van en `keywords[]`**; se reconocen con `reconocerDispositivo()` por prefijo.
- Permite varios dispositivos del mismo tipo en un mismo ambiente (`FOCO_sala`, `FOCO_cocina`).

### Sensores y atributos = palabras fijas

A diferencia de los dispositivos, los **sensores** (`SENSOR_TEMP`, `SENSOR_HUMEDAD`...) y los **atributos** (`ESTADO`, `BRILLO`...) son palabras reservadas fijas y **sí** están en `keywords[]`. Aunque `SENSOR_TEMP` contenga `_`, es una keyword fija, no un identificador dinámico.

### Orden de reconocimiento de una palabra

Al leer una palabra, `obtenerSiguienteToken()` clasifica en este orden:

1. ¿Es keyword fija? (`buscarKeyword`) → ese tipo.
2. ¿Coincide con prefijo de dispositivo? (`reconocerDispositivo`) → token de dispositivo.
3. Si no → `TK_ERROR`.

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

### 3. Verificar el lexer con `--tokens`

El parseo normal **todavía no es funcional** (los `parseBloque*` son stubs). Para inspeccionar lo que reconoce el lexer, usar el modo debug `--tokens`, que lexea el archivo e imprime cada token:

```bash
./lexer programa.dsl --tokens
```

Ejemplo de salida (formato `[línea:columna] TIPO 'lexema'`):

```
[1:0] TK_EVERY             'EVERY'
[1:6] TK_FOCO_ID           'FOCO_sala'
[1:16] TK_AIRE_ID          'AIRE_1'
[2:0] TK_WHEN              'WHEN'
[2:5] TK_SENSOR_TEMP       'SENSOR_TEMP'
[2:17] TK_ATRIB_ESTADO     'ESTADO'
[3:0] TK_ERROR             'FOCO_'      (sin sufijo → no es dispositivo)
[4:0] TK_EOF               ''
```

### 4. Qué esperar HOY (estado actual)

El lexer reconoce **palabras** (keywords fijas y dispositivos dinámicos), pero todavía **no** reconoce operadores ni literales (caen en `TK_ERROR`). Casos testeables:

| Caso | Comando | Salida esperada | Código de salida |
|------|---------|-----------------|------------------|
| Sin argumentos | `./lexer` | `Uso: ./lexer archivo.dsl [--tokens]` | `1` |
| Archivo inexistente | `./lexer no_existe.dsl --tokens` | `No se pudo abrir el archivo` | `1` |
| Lexeo de palabras | `./lexer programa.dsl --tokens` | Lista de tokens (ver arriba) | `0` |

Para ver el código de salida después de correr:

```bash
./lexer programa.dsl --tokens; echo "Exit: $?"
```

---

## Próximos pasos (en orden)

1. Lexeo de literales numéricos con unidad (`30°C`, `80%`, `h`, `min`, `lux`)
2. Lexeo de literales de texto (`"mensaje"`, emails, fechas, horas)
3. Implementar `parseBloqueWhen()` y el resto de funciones de parse
4. Habilitar el parseo normal (hoy solo funciona el modo `--tokens`)
