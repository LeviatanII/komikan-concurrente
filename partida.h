#ifndef PARTIDA_H
#define PARTIDA_H

#include <pthread.h>
#include <stdbool.h>

#include "tablero.h"
#include "config.h"

// Cantidad de hilos por partida: 1 puma y 12 perros
#define NUM_FICHAS (NUM_PERROS + 1)

// Identificador del puma dentro del arreglo de fichas
#define ID_PUMA -1

typedef enum {
    SIN_GANADOR,
    GANA_PUMA,
    GANA_PERROS
} Resultado;

typedef struct Partida Partida;

// Datos que recibe el hilo de cada ficha
typedef struct {
    Partida     *partida;
    int          id;        // ID_PUMA para el puma, 0..11 para los perros
    unsigned int semilla;   // semilla propia para rand_r
} Ficha;

struct Partida {
    int             id_mesa;
    const Config   *config;
    Tablero         tablero;

    // Orden de bloqueo: primero las casillas en orden ascendente y al final mutex_estado
    // Un mutex por casilla. Una ficha bloquea las casillas que lee o modifica
    pthread_mutex_t mutex_casilla[FILAS][COLUMNAS];

    // Protege terminada, resultado y movimientos
    pthread_mutex_t mutex_estado;
    pthread_cond_t  cond_fin;   // despierta a las fichas cuando la partida termina
    bool            terminada;
    Resultado       resultado;
    int             movimientos;

    Ficha           fichas[NUM_FICHAS];
    pthread_t       hilos[NUM_FICHAS];
};

// Juega una partida completa en la mesa indicada y retorna el resultado
Resultado partida_jugar(int id_mesa, const Config *cfg);

#endif
