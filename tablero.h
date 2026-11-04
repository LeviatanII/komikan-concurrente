#ifndef TABLERO_H
#define TABLERO_H

#include <stdbool.h>
#include <stdio.h>

/*
 * El tablero se representa como una matriz de 7 filas x 5 columnas.
 * Solo 28 de sus 35 casillas son nodos reales del tablero:
 *
 *   fila 0:          T            (vértice del triángulo, columna 2)
 *   fila 1:      .       .        (base del triángulo, columnas 1 y 3)
 *   filas 2-6:   .   .   .   .   .  (cuadrícula de 5 x 5)
 *
 * Las casillas que no son nodos se marcan con existe = false.
 */
#define FILAS      7
#define COLUMNAS   5
#define NUM_PERROS 12
#define MAX_VECINOS 8

// Cantidad de perros que el puma debe capturar para ganar
#define CAPTURAS_PARA_GANAR 6

typedef struct {
    int fila;
    int col;
} Pos;

typedef enum {
    VACIA,
    PUMA,
    PERRO
} Pieza;

typedef struct {
    bool  existe;   // true si la casilla es un nodo del tablero
    Pieza pieza;    // contenido de la casilla
    int   perro;    // identificador del perro (0..11) si pieza == PERRO
} Casilla;

typedef struct {
    Casilla casilla[FILAS][COLUMNAS];
    Pos     puma;                     // posicion actual del puma
    Pos     perros[NUM_PERROS];       // posicion actual de cada perro
    bool    perro_vivo[NUM_PERROS];   // false si el perro fue capturado
    int     capturados;               // perros capturados por el puma
} Tablero;

// Disposicion inicial por defecto: puma en el vertice del triangulo y perros en las dos ultimas filas mas las esquinas de la fila 4
void tablero_disposicion_defecto(Pos *puma, Pos perros[NUM_PERROS]);

// Inicializa el tablero con la disposicion indicada. Retorna false si alguna posicion no es un nodo o esta repetida
bool tablero_inicializar(Tablero *t, Pos puma, const Pos perros[NUM_PERROS]);

// Reglas de la geometria del tablero
bool tablero_es_nodo(Pos p);
bool tablero_adyacentes(Pos a, Pos b);
bool tablero_es_salto(Pos desde, Pos sobre, Pos hacia);

// Movimientos validos segun el estado actual. Retornan la cantidad de destinos escritos en el arreglo (como maximo MAX_VECINOS)
int tablero_movimientos_puma(const Tablero *t, Pos destinos[MAX_VECINOS]);
int tablero_movimientos_perro(const Tablero *t, int id, Pos destinos[MAX_VECINOS]);

// Aplican un movimiento si es valido. tablero_mover_puma escribe en capturado el id del perro capturado, o -1 si fue un movimiento simple
bool tablero_mover_puma(Tablero *t, Pos destino, int *capturado);
bool tablero_mover_perro(Tablero *t, int id, Pos destino);

// Imprime la matriz de juego: P = puma, o = perro, . = nodo vacio
void tablero_imprimir(const Tablero *t, FILE *salida);

#endif
