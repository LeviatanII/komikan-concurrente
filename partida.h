#ifndef PARTIDA_H
#define PARTIDA_H

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

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
    int 	 ciclos;    // Ciclos de desicion realizados (omito comentarios)
    int		 falla_en;  // Ciclo en que simula la falla, o -1 (Cositas)
    bool	 activa;    // Indica si el hilo sigue activo y tiene el teclado en espa;ol jejeje no tengo ñ... que?
} Ficha;

struct Partida {
    int             id_mesa;
    int             id_partida;
    const Config   *config;
    Tablero         tablero;
    FILE           *registro;   // archivo de log de la partida

    // Orden de bloqueo: primero las casillas en orden ascendente y al final mutex_estado
    // Un mutex por casilla. Una ficha bloquea las casillas que lee o modifica
    pthread_mutex_t mutex_casilla[FILAS][COLUMNAS];

    // Protege terminada, resultado, movimientos, eventos y el archivo de log
    pthread_mutex_t mutex_estado;
    pthread_cond_t  cond_fin;     // despierta a las fichas cuando la partida termina
    pthread_cond_t  cond_juez;    // despierta al juez cuando ocurre un evento
    bool            terminada;
    Resultado       resultado;
    int             movimientos;
    int             eventos;      // movimientos y avisos que el juez debe revisar
    bool 	    abandono;	  // Epic latin american dad moment
    bool	    interrumpida; // O abortada

    Ficha           fichas[NUM_FICHAS];
    pthread_t       hilos[NUM_FICHAS];
};

// Inicializa el tablero, abre el log y crea las primitivas de sincronizacion
bool partida_inicializar(Partida *p, int id_mesa, int id_partida, const Config *cfg);

// Cierra el log y destruye las primitivas de sincronizacion
void partida_destruir(Partida *p);

// Crea los hilos de las fichas y retorna cuantos se crearon
int partida_crear_fichas(Partida *p);

// Espera que terminen los hilos de las fichas creadas
void partida_esperar_fichas(Partida *p, int creados);

// Bloquea todas las casillas en orden ascendente y luego mutex_estado
void partida_bloquear_todo(Partida *p);

// Libera mutex_estado y todas las casillas
void partida_desbloquear_todo(Partida *p);

// Marca la partida como terminada y despierta a todas las fichas. Se llama con mutex_estado tomado
void partida_anunciar_fin(Partida *p, Resultado r);

// ABORTA la partida por cierre del servers
void partida_interrumpir(Partida *p);

// Cuenta los mutex que esta bloqueados
int partida_mutex_retenidos(Partida *p);

// Contador de mutex que estan bloqueados despues de unir las hilachas
int partida_mutex(Partida *p);

// Funciones de tiempo utilizadas por fichas
int partida_retardo_aleatorio(const Config *cfg, unsigned int *semilla);
void partida_tiempo_limite(int ms, struct timespec *limite);

#endif
