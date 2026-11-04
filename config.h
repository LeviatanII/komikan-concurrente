#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#include "tablero.h"

// Valores por defecto indicados en el enunciado
#define RETARDO_MIN_DEFECTO 50
#define RETARDO_MAX_DEFECTO 300

typedef struct {
    int          retardo_min;          // retardo minimo por movimiento
    int          retardo_max;          // retardo maximo por movimiento
    unsigned int semilla;              // semilla aleatoria
    Pos          puma;                 // posicion inicial del puma
    Pos          perros[NUM_PERROS];   // posiciones iniciales de los perros
} Config;

// Carga los valores por defecto. La semilla se obtiene de la hora actual.
void config_defecto(Config *cfg);

// Lee el archivo de configuracion y reemplaza los valores indicados en el.
// Retorna false e informa el error por stderr si el archivo es invalido.
bool config_cargar(const char *ruta, Config *cfg);

// Muestra la configuracion utilizada
void config_mostrar(const Config *cfg);

#endif
