# Smart Home Lexer

Analizador léxico para un lenguaje de domótica. Toma un script de texto con reglas de automatización y produce una secuencia de tokens.

## Ejemplo de input

```
WHEN sensor_luz < 250lux DO
    foco_entrada.estado = ON
    foco_entrada.brillo = 80%
END

EVERY 30m DO
    IF reloj_principal.hora > 22:00 THEN
        persiana_sala.posicion = 0%
        altavoz_comedor.mensaje = "Son las 22hs"
        altavoz_comedor.email_notif = notif@casa.com.ar
    END
END
```

## Tokens reconocidos

### Palabras reservadas

| Token | Lexema |
|-------|--------|
| `TK_WHEN` | `WHEN` |
| `TK_DO` | `DO` |
| `TK_END` | `END` |
| `TK_IF` | `IF` |
| `TK_THEN` | `THEN` |
| `TK_ELSE` | `ELSE` |
| `TK_EVERY` | `EVERY` |
| `TK_AND` | `AND` |
| `TK_OR` | `OR` |
| `TK_NOT` | `NOT` |

> No se distingue entre mayúsculas y minúsculas: `WHEN`, `when` y `When` son equivalentes.

### Sensores

| Token | Lexema |
|-------|--------|
| `TK_SENSOR_TEMP` | `sensor_temp` |
| `TK_SENSOR_HUMEDAD` | `sensor_humedad` |
| `TK_SENSOR_LUZ` | `sensor_luz` |
| `TK_SENSOR_MOVIMIENTO` | `sensor_movimiento` |
| `TK_SENSOR_HUMO` | `sensor_humo` |

### Dispositivos

Se reconocen por prefijo seguido de un sufijo alfanumérico (ej: `foco_sala`, `aire_1`).

| Token | Prefijo |
|-------|---------|
| `TK_FOCO_ID` | `foco_` |
| `TK_AIRE_ID` | `aire_` |
| `TK_PERSIANA_ID` | `persiana_` |
| `TK_CERRADURA_ID` | `cerradura_` |
| `TK_RELOJ_ID` | `reloj_` |
| `TK_ALTAVOZ_ID` | `altavoz_` |
| `TK_ALARMA_ID` | `alarma_` |

### Atributos

`estado`, `brillo`, `color`, `temp_obj`, `temp_act`, `modo`, `posicion`, `hora`, `fecha`, `mute`, `volumen`, `email_notif`, `mensaje`, `activada`

### Literales

| Token | Formato | Ejemplos |
|-------|---------|---------|
| `TK_TEMP` | entero + `°C` | `25°C`, `0°C` |
| `TK_PORCENTAJE` | entero + `%` | `80%`, `100%` |
| `TK_TIEMPO` | entero + `s`/`m`/`min`/`h` | `10s`, `30m`, `2h` |
| `TK_LUX` | entero + `lux` | `500lux` |
| `TK_HORA` | `HH:MM` (24h) | `08:30`, `23:59` |
| `TK_FECHA` | `DD/MM/AAAA` | `21/04/2026` |
| `TK_TEXTO` | entre comillas dobles | `"Hola mundo"` |
| `TK_EMAIL` | `usuario@dominio.ext` | `notif@casa.com.ar` |
| `TK_BOOL_SENSOR` | `TRUE` / `FALSE` | para sensores |
| `TK_BOOL_ACTUADOR` | `ON` / `OFF` | para actuadores |
| `TK_MODO` | `FRIO` / `CALOR` / `VENT` | modo del aire |
| `TK_COLOR` | `blanco` / `rojo` / `azul` | color del foco |

### Operadores y delimitadores

| Token | Símbolo |
|-------|---------|
| `TK_IGUAL` | `==` |
| `TK_DIFERENTE` | `!=` |
| `TK_MAYOR` | `>` |
| `TK_MAYORIGUAL` | `>=` |
| `TK_MENOR` | `<` |
| `TK_MENORIGUAL` | `<=` |
| `TK_ASIGNACION` | `=` |
| `TK_DELIMITADOR` | `.` |
| `TK_PAR_IZQ` | `(` |
| `TK_PAR_DER` | `)` |

### Comentarios

Las líneas que empiezan con `//` son ignoradas por el lexer.

## Compilar

### Requisitos

| Sistema | Comando |
|---------|---------|
| Linux (Ubuntu/Debian) | `sudo apt install build-essential` |
| Linux (Fedora/RHEL) | `sudo dnf install make gcc` |
| macOS | `xcode-select --install` |
| Windows | WSL recomendado |


### Comandos

```bash
make          # compila smart
make clean    # elimina los binarios
```

## Uso

El ejecutable admite distintos modos de funcionamiento según la etapa del compilador que se desee ejecutar.

### Modo interactivo

Inicia un tokenizador interactivo. Cada línea ingresada se analiza y se muestran los tokens generados.

```bash
./smart
```

Salir escribiendo:

```text
exit
```

---

### Análisis léxico

Tokeniza un archivo fuente y muestra todos los tokens reconocidos.

```bash
./smart tokens programa.smart
```
### Modo lexer

Lexea un archivo e imprime cada token con su posición:

Salida (formato `[línea:columna] TIPO 'lexema'`):

```
[1:0]  TK_WHEN         'WHEN'
[1:5]  TK_SENSOR_LUZ   'sensor_luz'
[1:16] TK_MENOR        '<'
[1:18] TK_LUX          '250lux'
[1:25] TK_DO           'DO'
```

---

### Análisis sintáctico

Ejecuta el parser mostrando el proceso de reconocimiento mediante los mensajes `MATCH`.

```bash
./smart parse programa.smart
```

---

### Visualización del AST

Construye e imprime el Árbol de Sintaxis Abstracta (AST).

```bash
./smart ast programa.smart
```

---

### Generación de HTML

Interpreta el programa y genera el archivo `salida.html` con el estado resultante del sistema.

```bash
./smart html programa.smart
```

---

### Ejecución completa

Realiza todo el proceso:

1. Análisis léxico.
2. Análisis sintáctico.
3. Construcción del AST.
4. Interpretación.
5. Generación de `salida.html`.
6. Apertura automática del archivo HTML en el navegador predeterminado.

Utilizando el archivo por defecto (`programa1.smart`):

```bash
./smart run
```

O indicando un archivo:

```bash
./smart run programa2.smart
```

---

### Mostrar ayuda

El ejecutable incorpora un sistema de ayuda integrado que puede consultarse desde la línea de comandos.

```bash
./smart help
```

También son válidas las siguientes opciones:

```bash
./smart -h
./smart --help
```

---

## Restricciones

Los archivos fuente deben poseer la extensión `.smart`.

Ejemplo válido ya incorporados en el directorio:

```text
programa1.smart
programa2.smart
programa3.smart
```

Ejemplos inválidos:

```text
programa1.txt
programa1
config.dsl
```
