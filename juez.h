#ifndef JUEZ_H
#define JUEZ_H

#include "partida.h"
#include "config.h"

// Datos que recibe el hilo juez de una mesa
typedef struct {
    int           id_mesa;
    int           id_partida;
    const Config *config;

    pthread_mutex_t *mutex_servidor;
    Partida **registro;
    const bool *cerrando;


    Resultado     resultado;   // lo escribe el juez al terminar la partida
    bool abandono;
    bool interrumpida;

} DatosJuez;

// Hilo juez: inicializa la partida, crea las fichas, revisa el tablero tras cada evento, detecta la victoria y libera los recursos
void *hilo_juez(void *arg);

// Texto que describe un resultado
const char *juez_texto_resultado(Resultado r, bool abandono, bool interrumpida);

#endif
