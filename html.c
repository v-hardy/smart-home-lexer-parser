#include <stdio.h>
#include <string.h>

#include "html.h"

/*------------------------------------------
    Email
-------------------------------------------*/

static void escribirValor(FILE *f, const char *atributo, const char *valor)
{
    if (strcmp(atributo, "emailNotif") == 0 ||
        strcmp(atributo, "EMAIL_NOTIF") == 0)
    {
        char usuario[128];

        strcpy(usuario, valor);

        char *p = strchr(usuario, '@');

        if(p)
            *p = '\0';

        fprintf(f,
                "<li>%s: "
                "<a href=\"mailto:%s\">Contactar a %s</a>"
                "</li>\n",
                atributo,
                valor,
                usuario);
    }
    else
    {
        fprintf(f,
                "<li>%s: %s</li>\n",
                atributo,
                valor);
    }
}

/*------------------------------------------
    HTML
-------------------------------------------*/

void generarHTML(const char *archivoSalida, EstadoSistema *estado)
{
    FILE *f = fopen(archivoSalida, "w");

    if(!f)
        return;

    fprintf(f,
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<meta charset=\"UTF-8\">\n"
        "<title>Smart Home</title>\n"
        "</head>\n"
        "<body>\n");

    /*============================
      Sensores
    ============================*/

    fprintf(f,
        "<div style=\"border:1px solid green;"
        "padding:20px;\">\n");

    fprintf(f,
        "<h1>Estado de Sensores</h1>\n");

    fprintf(f,
        "<h2>Temperatura</h2>\n"
        "<p>%.1f °C</p>\n",
        estado->sensores.temperatura);

    fprintf(f,
        "<h2>Humedad</h2>\n"
        "<p>%.0f %%</p>\n",
        estado->sensores.humedad);

    fprintf(f,
        "<h2>Luz</h2>\n"
        "<p>%.0f lux</p>\n",
        estado->sensores.luz);

    fprintf(f,
        "<h2>Movimiento</h2>\n"
        "<p>%s</p>\n",
        estado->sensores.movimiento ? "true" : "false");

    fprintf(f,
        "<h2>Humo</h2>\n"
        "<p>%s</p>\n",
        estado->sensores.humo ? "true" : "false");

    fprintf(f,"</div>\n");

    /*============================
      Actuadores
    ============================*/

    Actuador *a = estado->actuadores;

    while(a)
    {
        fprintf(f,
            "<div style=\"border:1px solid gray;"
            "padding:20px;"
            "margin-top:20px;\">\n");

        fprintf(f,
            "<h1>%s</h1>\n",
            a->nombre);

        fprintf(f,"<ul>\n");

        Atributo *atr = a->atributos;

        while(atr)
        {
            escribirValor(f, atr->nombre, atr->valor.lexema);
            atr = atr->sig;
        }

        fprintf(f,"</ul>\n");

        fprintf(f,"</div>\n");
        
        a = a->sig;
    }

    fprintf(f,
        "</body>\n"
        "</html>\n");

    fclose(f);
}