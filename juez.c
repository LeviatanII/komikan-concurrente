#include <stdio.h>

#include "juez.h"

const char *juez_texto_resultado(Resultado r) {
    switch (r) {
    case GANA_PUMA:   return "gana el puma";
    case GANA_PERROS: return "ganan los perros";
    default:          return "sin ganador";
    }
}

// Comprueba que el tablero sea consistente: un solo puma y cada perro en juego en la casilla que tiene registrada
static bool estado_consistente(const Tablero *t) {
    int pumas = 0, perros = 0;
    for (int f = 0; f < FILAS; f++) {
        for (int c = 0; c < COLUMNAS; c++) {
            const Casilla *cs = &t->casilla[f][c];
            if (cs->pieza == VACIA) continue;
            if (!cs->existe) return false;
            if (cs->pieza == PUMA) {
                pumas++;
                if (t->puma.fila != f || t->puma.col != c) return false;
            } else {
                perros++;
                int id = cs->perro;
                if (id < 0 || id >= NUM_PERROS || !t->perro_vivo[id]) return false;
                if (t->perros[id].fila != f || t->perros[id].col != c) return false;
            }
        }
    }
    return pumas == 1 && perros == NUM_PERROS - t->capturados;
}

// Toma una instantanea consistente del tablero completo, la registra en el log y revisa las condiciones de victoria
static void revisar_tablero(Partida *p) {
    partida_bloquear_todo(p);
    if (!p->terminada) {
        fprintf(p->registro, "Tablero tras %d movimientos:\n", p->movimientos);
        tablero_imprimir(&p->tablero, p->registro);

        Pos destinos[MAX_VECINOS];
        if (!estado_consistente(&p->tablero)) {
            fprintf(p->registro, "Estado inconsistente, se aborta la partida\n");
            partida_anunciar_fin(p, SIN_GANADOR);
        } else if (p->tablero.capturados >= CAPTURAS_PARA_GANAR) {
            partida_anunciar_fin(p, GANA_PUMA);
        } else if (tablero_movimientos_puma(&p->tablero, destinos) == 0) {
            fprintf(p->registro, "El puma quedo acorralado\n");
            partida_anunciar_fin(p, GANA_PERROS);
        }
    }
    partida_desbloquear_todo(p);
}

void *hilo_juez(void *arg) {
    DatosJuez *d = arg;
    d->resultado = SIN_GANADOR;

    // El juez inicializa el tablero y las primitivas de sincronizacion
    Partida p;
    if (!partida_inicializar(&p, d->id_mesa, d->id_partida, d->config))
        return NULL;

    printf("[mesa %d] Comienza la partida %d (log en partida_%d.log)\n",
           d->id_mesa, d->id_partida, d->id_partida);
    fprintf(p.registro, "Partida %d en la mesa %d\n", d->id_partida, d->id_mesa);
    revisar_tablero(&p);   // registra el tablero inicial

    int creados = partida_crear_fichas(&p);
    if (creados < NUM_FICHAS) {
        pthread_mutex_lock(&p.mutex_estado);
        partida_anunciar_fin(&p, SIN_GANADOR);
        pthread_mutex_unlock(&p.mutex_estado);
    }

    // El juez duerme hasta que una ficha avisa un evento y entonces revisa el tablero
    int revisados = 0;
    while (true) {
        pthread_mutex_lock(&p.mutex_estado);
        while (!p.terminada && p.eventos == revisados)
            pthread_cond_wait(&p.cond_juez, &p.mutex_estado);
        bool fin = p.terminada;
        revisados = p.eventos;
        pthread_mutex_unlock(&p.mutex_estado);

        if (fin) break;
        revisar_tablero(&p);
    }

    // Se espera a todas las fichas antes de liberar la mesa
    partida_esperar_fichas(&p, creados);

    fprintf(p.registro, "Fin de la partida tras %d movimientos: %s\n",
            p.movimientos, juez_texto_resultado(p.resultado));
    printf("[mesa %d] Fin de la partida %d tras %d movimientos: %s\n",
           d->id_mesa, d->id_partida, p.movimientos, juez_texto_resultado(p.resultado));

    d->resultado = p.resultado;
    partida_destruir(&p);
    return NULL;
}
