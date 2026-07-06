#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// --- FUNCIONES AUXILIARES DE PARSEO ---

// Verifica si una cadena contiene solo dígitos numéricos
static bool esDígitos(const char *s) {
    if (*s == '\0') return false;
    while (*s) {
        if (!isdigit((unsigned char)*s)) return false;
        s++;
    }
    return true;
}

// Verifica si un carácter es válido para usuario/dominio de Email (letras, números, '_', '.', '-')
static bool esCaracEmail(char c) {
    return isalnum((unsigned char)c) || c == '_' || c == '.' || c == '-';
}


// --- 1. VALIDACIÓN DE TEXTO ---
bool validarTexto(const char *s) {
    if (s == NULL) return false;
    size_t len = strlen(s);
    if (len < 2) return false;
    return (s[0] == '"' && s[len - 1] == '"');
}


// --- 2. VALIDACIÓN DE BOOLEANOS ---
bool validarBoolSensor(const char *s) {
    if (s == NULL) return false;
    return (strcmp(s, "TRUE") == 0 || strcmp(s, "FALSE") == 0);
}

bool validarBoolDispositivo(const char *s) {
    if (s == NULL) return false;
    return (strcmp(s, "ON") == 0 || strcmp(s, "OFF") == 0);
}


// --- 3. VALIDACIÓN DE TEMPERATURA ---
bool validarTemperaturaRango(const char *s, double minimo, double maximo) {
    if (s == NULL) return false;

    // Buscar el sufijo "°C". Puede ser UTF-8 (2 bytes para °) o ASCII extendido (1 byte)
    const char *sufijo = strstr(s, "°C");
    if (!sufijo || *(sufijo + strlen("°C")) != '\0') return false;

    // Aislar la parte numérica
    size_t num_len = sufijo - s;
    if (num_len == 0) return false;

    char buffer[64];
    if (num_len >= sizeof(buffer)) return false;

    strncpy(buffer, s, num_len);
    buffer[num_len] = '\0';

    // Validar formato del número manual
    const char *p = buffer;

    if (*p == '+' || *p == '-') p++;
    if (*p == '\0') return false;

    bool tiene_punto = false;
    const char *inicio_num = p;

    while (*p) {
        if (*p == '.') {
            if (tiene_punto) return false;
            tiene_punto = true;
        } else if (!isdigit((unsigned char)*p)) {
            return false;
        }
        p++;
    }

    if (*(p - 1) == '.') return false;

    double valor = strtod(buffer, NULL);
    return (valor >= minimo && valor <= maximo);
}

bool validarTemperatura(const char *s) {
    return validarTemperaturaRango(s, -1.79e308, 1.79e308);
}


// --- 4. VALIDACIÓN DE PORCENTAJE ---
bool validarPorcentaje(const char *s) {
    if (s == NULL) return false;

    size_t len = strlen(s);
    if (len < 2 || s[len - 1] != '%') return false;

    char buffer[32];
    if (len >= sizeof(buffer)) return false;

    strncpy(buffer, s, len - 1);
    buffer[len - 1] = '\0';

    if (!esDígitos(buffer)) return false;

    long valor = strtol(buffer, NULL, 10);
    return (valor >= 0 && valor <= 100);
}


// --- 5. VALIDACIÓN DE TIEMPO ---
bool validarTiempo(const char *s) {
    if (s == NULL) return false;

    size_t len = strlen(s);
    if (len < 2) return false;

    char unidad = s[len - 1];
    if (unidad != 's' && unidad != 'm' && unidad != 'h') return false;

    char buffer[32];
    if (len >= sizeof(buffer)) return false;

    strncpy(buffer, s, len - 1);
    buffer[len - 1] = '\0';

    return esDígitos(buffer);
}


// --- 6. VALIDACIÓN DE ILUMINANCIA ---
bool validarIluminancia(const char *s) {
    if (s == NULL) return false;

    size_t len = strlen(s);
    if (len < 4) return false;

    if (strcmp(s + len - 3, "lux") != 0) return false;

    char buffer[32];
    if (len - 3 >= sizeof(buffer)) return false;

    strncpy(buffer, s, len - 3);
    buffer[len - 3] = '\0';

    if (!esDígitos(buffer)) return false;

    long valor = strtol(buffer, NULL, 10);
    return (valor >= 0 && valor <= 1000);
}


// --- 7. VALIDACIÓN DE HORA ---
bool validarHora(const char *s) {
    if (s == NULL || strlen(s) != 5) return false;
    if (s[2] != ':') return false;

    if (!isdigit((unsigned char)s[0]) ||
        !isdigit((unsigned char)s[1]) ||
        !isdigit((unsigned char)s[3]) ||
        !isdigit((unsigned char)s[4]))
        return false;

    int hh = (s[0] - '0') * 10 + (s[1] - '0');
    int mm = (s[3] - '0') * 10 + (s[4] - '0');

    return (hh >= 0 && hh <= 23 && mm >= 0 && mm <= 59);
}


// --- 8. VALIDACIÓN DE FECHA ---
bool validarFechaRango(const char *s, int diaMin, int mesMin, int anioMin, int diaMax, int mesMax, int anioMax) {
    if (s == NULL || strlen(s) != 10) return false;
    
    // CORRECCIÓN: Las barras '/' están en las posiciones indexadas 2 y 5
    if (s[2] != '/' || s[5] != '/') return false;
    
    // Validar que todos los demás caracteres sean estrictamente dígitos
    for (int i = 0; i < 10; i++) {
        if (i == 2 || i == 5) continue; 
        if (!isdigit((unsigned char)s[i])) return false;
    }
    
    // Extracción manual de componentes numéricos
    int d = (s[0] - '0') * 10 + (s[1] - '0');
    int m = (s[3] - '0') * 10 + (s[4] - '0');
    int a = (s[6] - '0') * 1000 + (s[7] - '0') * 100 + (s[8] - '0') * 10 + (s[9] - '0');
    
    // Restricciones lógicas generales base
    if (d < 1 || d > 31 || m < 1 || m > 12 || a < 1900 || a > 2099) return false;
    
    // Conversión lineal para comparación de rangos (AAAAMMDD)
    long fechaActual = a * 10000L + m * 100 + d;
    long fechaMin = anioMin * 10000L + mesMin * 100 + diaMin;
    long fechaMax = anioMax * 10000L + mesMax * 100 + diaMax;
    
    return (fechaActual >= fechaMin && fechaActual <= fechaMax);
}

bool validarFecha(const char *s) {
    return validarFechaRango(s, 1, 1, 1900, 31, 12, 2099);
}


// --- 9. VALIDACIÓN DE EMAIL ---
bool validarEmail(const char *s) {
    if (s == NULL) return false;

    const char *arroba = strchr(s, '@');
    if (!arroba || strchr(arroba + 1, '@') != NULL) return false;

    if (arroba == s) return false;

    for (const char *p = s; p < arroba; p++) {
        if (!esCaracEmail(*p)) return false;
    }

    const char *ultimo_punto = strrchr(arroba + 1, '.');
    if (!ultimo_punto || ultimo_punto == arroba + 1) return false;

    for (const char *p = arroba + 1; p < ultimo_punto; p++) {
        if (!esCaracEmail(*p)) return false;
    }

    size_t ext_len = strlen(ultimo_punto + 1);
    if (ext_len < 2 || ext_len > 4) return false;

    for (const char *p = ultimo_punto + 1; *p != '\0'; p++) {
        if (!isalpha((unsigned char)*p)) return false;
    }

    return true;
}


// --- TIPOS DISCRETOS ---
bool validarColor(const char *s) {
    if (s == NULL) return false;

    return (strcmp(s, "blanco") == 0 ||
            strcmp(s, "rojo") == 0 ||
            strcmp(s, "azul") == 0);
}

bool validarModoAire(const char *s) {
    if (s == NULL) return false;

    return (strcmp(s, "FRIO") == 0 ||
            strcmp(s, "CALOR") == 0 ||
            strcmp(s, "VENT") == 0);
}


// --- 10. FUNCIÓN GENERAL DE ROUTING (validarValor) ---
bool validarValor(const char *tipo,
                  const char *atributo,
                  const char *valor) {
    if (!tipo || !atributo || !valor) return false;

    // --- SENSORES ---
    if (strcmp(tipo, "sensor_temp") == 0 &&
        strcmp(atributo, "temperatura") == 0) {
        return validarTemperaturaRango(valor, -10.0, 50.0);
    }

    if (strcmp(tipo, "sensor_humedad") == 0 &&
        strcmp(atributo, "porcentaje") == 0) {
        return validarPorcentaje(valor);
    }

    if (strcmp(tipo, "sensor_luz") == 0 &&
        strcmp(atributo, "iluminancia") == 0) {
        return validarIluminancia(valor);
    }

    if ((strcmp(tipo, "sensor_movimiento") == 0 ||
         strcmp(tipo, "sensor_humo") == 0) &&
        strcmp(atributo, "estado") == 0) {
        return validarBoolSensor(valor);
    }

    // --- DISPOSITIVOS ---

    // Foco
    if (strcmp(tipo, "foco_") == 0) {
        if (strcmp(atributo, "estado") == 0)
            return validarBoolDispositivo(valor);

        if (strcmp(atributo, "brillo") == 0)
            return validarPorcentaje(valor);

        if (strcmp(atributo, "color") == 0)
            return validarColor(valor);
    }

    // Aire acondicionado
    if (strcmp(tipo, "aire_") == 0) {
        if (strcmp(atributo, "estado") == 0)
            return validarBoolDispositivo(valor);

        if (strcmp(atributo, "modo") == 0)
            return validarModoAire(valor);

        if (strcmp(atributo, "temp_obj") == 0)
            return validarTemperaturaRango(valor, 16.0, 30.0);

        if (strcmp(atributo, "temp_act") == 0)
            return validarTemperaturaRango(valor, -10.0, 50.0);
    }

    // Persiana
    if (strcmp(tipo, "persiana_") == 0 &&
        strcmp(atributo, "posicion") == 0) {
        return validarPorcentaje(valor);
    }

    // Cerradura
    if (strcmp(tipo, "cerradura_") == 0 &&
        strcmp(atributo, "estado") == 0) {
        return validarBoolDispositivo(valor);
    }

    // Reloj
    if (strcmp(tipo, "reloj_") == 0) {
        if (strcmp(atributo, "hora") == 0)
            return validarHora(valor);

        if (strcmp(atributo, "fecha") == 0)
            return validarFechaRango(valor, 1, 1, 2000, 31, 12, 2099);
    }

    // Altavoz
    if (strcmp(tipo, "altavoz_") == 0) {
        if (strcmp(atributo, "volumen") == 0)
            return validarPorcentaje(valor);

        if (strcmp(atributo, "mute") == 0)
            return validarBoolDispositivo(valor);

        if (strcmp(atributo, "mensaje") == 0)
            return validarTexto(valor);

        if (strcmp(atributo, "email_notif") == 0)
            return validarEmail(valor);
    }

    // Alarma
    if (strcmp(tipo, "alarma_") == 0) {
        if (strcmp(atributo, "estado") == 0 ||
            strcmp(atributo, "activada") == 0) {
            return validarBoolDispositivo(valor);
        }
    }

    return false;
}


// --- 11. BATERÍA DE PRUEBAS (MAIN) ---
void test(const char *categoria, bool resultado, const char *desc) {
    printf("[%s] %s -> %s\n",
           resultado ? "OK" : "FAIL",
           categoria,
           desc);
}

int main(void) {
    printf("=== INICIANDO PRUEBAS DEL LEXER DSL ===\n\n");

    // Pruebas Texto
    test("Texto Válido", validarTexto("\"Hola Mundo #12\""), "\"Hola Mundo #12\"");
    test("Texto Vacío", validarTexto("\"\""), "\"\"");
    test("Texto Inválido", !validarTexto("Hola\""), "Falta comilla inicial");

    // Pruebas Booleanos
    test("Bool Sensor OK", validarBoolSensor("TRUE"), "TRUE");
    test("Bool Sensor Fail", !validarBoolSensor("true"), "Minusculas");
    test("Bool Dispositivo OK", validarBoolDispositivo("ON"), "ON");

    // Pruebas Temperatura
    test("Temp Básica", validarTemperatura("25°C"), "25°C");
    test("Temp Negativa", validarTemperatura("-5.5°C"), "-5.5°C");
    test("Temp Rango AC", validarTemperaturaRango("22°C", 16.0, 30.0), "22°C en rango [16, 30]");
    test("Temp Rango AC Fail", !validarTemperaturaRango("15°C", 16.0, 30.0), "15°C fuera de rango");

    // Pruebas Porcentaje
    test("Porcentaje 50%", validarPorcentaje("50%"), "50%");
    test("Porcentaje 101%", !validarPorcentaje("101%"), "Fuera de rango máximo");

    // Pruebas Tiempo
    test("Tiempo 10s", validarTiempo("10s"), "10s");
    test("Tiempo Error", !validarTiempo("-5m"), "No enteros negativos");

    // Pruebas Iluminancia
    test("Iluminancia OK", validarIluminancia("600lux"), "600lux");
    test("Iluminancia Fail", !validarIluminancia("1200lux"), "Excede los 1000 lux");

    // Pruebas Hora
    test("Hora OK", validarHora("23:59"), "23:59");
    test("Hora Fail", !validarHora("24:00"), "Hora inexistente");

    // Pruebas Fecha
    test("Fecha General", validarFecha("15/08/2026"), "15/08/2026");
    test("Fecha Reloj Fail",
         !validarFechaRango("01/01/1995", 1, 1, 2000, 31, 12, 2099),
         "1995 queda fuera del rango reloj");

    // Pruebas Email
    test("Email Complejo", validarEmail("ana.perez@correo.edu"), "ana.perez@correo.edu");
    test("Email Malo", !validarEmail("juan@gmail.c"), "Extensión muy corta");

    // Pruebas de Router (validarValor)
    test("Router Sensor Temp",
         validarValor("sensor_temp", "temperatura", "45°C"),
         "sensor_temp temp=45°C (Válido)");

    test("Router Sensor Temp Extremo",
         !validarValor("sensor_temp", "temperatura", "55°C"),
         "sensor_temp temp=55°C (Límite es 50)");

    test("Router Foco Color",
         validarValor("foco_", "color", "azul"),
         "foco_ color=azul (Discreto válido)");

    test("Router Aire Modo Fail",
         !validarValor("aire_", "modo", "INVIERNO"),
         "aire_ modo=INVIERNO (No existe)");

    printf("\n=== PRUEBAS FINALIZADAS ===\n");

    return 0;
}