#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>

#include "config.h"

#define LARGO_LINEA 512

void config_defecto(Config *cfg) {
    cfg->retardo_min = RETARDO_MIN_DEFECTO;
    cfg->retardo_max = RETARDO_MAX_DEFECTO;
    cfg->semilla = (unsigned int)time(NULL);
    tablero_disposicion_defecto(&cfg->puma, cfg->perros);
}

// Elimina los espacios al inicio y al final de s
static char *recortar(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *fin = s + strlen(s);
    while (fin > s && isspace((unsigned char)fin[-1])) fin--;
    *fin = '\0';
    return s;
}

// Lee un entero desde *s y avanza el puntero hasta el final del numero
static bool leer_entero(char **s, long *valor) {
    char *fin;
    errno = 0;
    long v = strtol(*s, &fin, 10);
    if (fin == *s || errno != 0) return false;
    *valor = v;
    *s = fin;
    return true;
}

// Indica si en s solo quedan espacios
static bool solo_espacios(const char *s) {
    while (isspace((unsigned char)*s)) s++;
    return *s == '\0';
}

// Lee un unico entero entre min y max sin caracteres sobrantes
static bool leer_valor(char *s, long min, long max, long *valor) {
    return leer_entero(&s, valor) && solo_espacios(s) &&
           *valor >= min && *valor <= max;
}

// Lee una posicion con formato fila,col y comprueba que sea un nodo
static bool leer_pos(char **s, Pos *p) {
    long f, c;
    if (!leer_entero(s, &f) || **s != ',') return false;
    (*s)++;
    if (!leer_entero(s, &c)) return false;
    if (f < 0 || f >= FILAS || c < 0 || c >= COLUMNAS) return false;
    p->fila = (int)f;
    p->col = (int)c;
    return tablero_es_nodo(*p);
}

// Lee exactamente NUM_PERROS posiciones separadas por espacios
static bool leer_perros(char *s, Pos perros[NUM_PERROS]) {
    for (int i = 0; i < NUM_PERROS; i++) {
        while (isspace((unsigned char)*s)) s++;
        if (!leer_pos(&s, &perros[i])) return false;
    }
    return solo_espacios(s);
}

// Comprueba que no existan dos fichas en la misma posicion
static bool posiciones_unicas(const Config *cfg) {
    Pos todas[NUM_PERROS + 1];
    todas[0] = cfg->puma;
    for (int i = 0; i < NUM_PERROS; i++) todas[i + 1] = cfg->perros[i];
    for (int i = 0; i <= NUM_PERROS; i++)
        for (int j = i + 1; j <= NUM_PERROS; j++)
            if (todas[i].fila == todas[j].fila && todas[i].col == todas[j].col)
                return false;
    return true;
}

// Procesa una linea clave = valor. Retorna un mensaje de error o NULL.
static const char *procesar_linea(char *linea, Config *cfg, bool vistos[5]) {
    static const char *claves[5] = {
        "retardo_min", "retardo_max", "semilla", "puma", "perros"
    };

    char *igual = strchr(linea, '=');
    if (igual == NULL) return "se esperaba clave = valor";
    *igual = '\0';
    char *clave = recortar(linea);
    char *valor = recortar(igual + 1);

    int k = -1;
    for (int i = 0; i < 5; i++)
        if (strcmp(clave, claves[i]) == 0) k = i;
    if (k == -1) return "clave desconocida";
    if (vistos[k]) return "clave repetida";
    vistos[k] = true;

    long v;
    switch (k) {
    case 0:
        if (!leer_valor(valor, 0, INT_MAX, &v)) return "retardo_min invalido";
        cfg->retardo_min = (int)v;
        break;
    case 1:
        if (!leer_valor(valor, 0, INT_MAX, &v)) return "retardo_max invalido";
        cfg->retardo_max = (int)v;
        break;
    case 2:
        if (!leer_valor(valor, 0, INT_MAX, &v)) return "semilla invalida";
        cfg->semilla = (unsigned int)v;
        break;
    case 3:
        if (!leer_pos(&valor, &cfg->puma) || !solo_espacios(valor))
            return "posicion del puma invalida";
        break;
    case 4:
        if (!leer_perros(valor, cfg->perros))
            return "se esperaban 12 posiciones validas para los perros";
        break;
    }
    return NULL;
}

bool config_cargar(const char *ruta, Config *cfg) {
    FILE *f = fopen(ruta, "r");
    if (f == NULL) {
        fprintf(stderr, "No se pudo abrir %s: %s\n", ruta, strerror(errno));
        return false;
    }

    char linea[LARGO_LINEA];
    bool vistos[5] = {false, false, false, false, false};
    int num_linea = 0;
    const char *error = NULL;

    while (error == NULL && fgets(linea, sizeof(linea), f) != NULL) {
        num_linea++;
        if (strchr(linea, '\n') == NULL && !feof(f)) {
            error = "linea demasiado larga";
            break;
        }
        char *texto = recortar(linea);
        if (texto[0] == '\0' || texto[0] == '#') continue;
        error = procesar_linea(texto, cfg, vistos);
    }
    fclose(f);

    if (error != NULL) {
        fprintf(stderr, "%s:%d: %s\n", ruta, num_linea, error);
        return false;
    }
    if (cfg->retardo_min > cfg->retardo_max) {
        fprintf(stderr, "%s: retardo_min no puede ser mayor que retardo_max\n", ruta);
        return false;
    }
    if (!posiciones_unicas(cfg)) {
        fprintf(stderr, "%s: hay fichas repetidas en la misma posicion\n", ruta);
        return false;
    }
    return true;
}

void config_mostrar(const Config *cfg) {
    printf("[torneo] Retardo por movimiento: %d a %d ms\n",
           cfg->retardo_min, cfg->retardo_max);
    printf("[torneo] Semilla: %u\n", cfg->semilla);
}
