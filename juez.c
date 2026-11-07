
#include <stdio.h>

#include "juez.h"

const char *juez_texto_resultado(Resultado r, bool abandono,
                                  bool interrumpida) {
    if (interrumpida)
        return "partida interrumpida";

    if (r == GANA_PUMA)
        return abandono ? "gana el puma por abandono de los perros"
                        : "gana el puma";

    if (r == GANA_PERROS)
        return abandono ? "ganan los perros por abandono del puma"
                        : "ganan los perros";

    return "sin ganador";
}

// Busca una ficha que terminó inesperadamente.
static int buscar_falla(const Partida *p) {
    for (int i = 0; i < NUM_FICHAS; i++) {
        const Ficha *f = &p->fichas[i];

        if (f->activa)
            continue;

        // Un perro capturado terminó normalmente.
        if (f->id != ID_PUMA && !p->tablero.perro_vivo[f->id])
            continue;

        return i;
    }

    return -1;
}

// Aborta la partida y declara ganador al oponente.
static void declarar_abandono(Partida *p, int ficha) {
    bool fallo_puma = p->fichas[ficha].id == ID_PUMA;

    p->abandono = true;

    const char *jugador = fallo_puma ? "del puma" : "de los perros";

    fprintf(p->registro,
            "El juez detecta que el jugador %s fallo y aborta la partida\n",
            jugador);

    printf("[mesa %d] El jugador %s abandona la partida %d\n",
           p->id_mesa, jugador, p->id_partida);

    partida_anunciar_fin(p, fallo_puma ? GANA_PERROS : GANA_PUMA);
}

// Comprueba que el tablero mantenga su consistencia.
static bool estado_consistente(const Tablero *t) {
    int pumas = 0;
    int perros = 0;

    for (int f = 0; f < FILAS; f++) {
        for (int c = 0; c < COLUMNAS; c++) {
            const Casilla *cs = &t->casilla[f][c];

            if (cs->pieza == VACIA)
                continue;

            if (!cs->existe)
                return false;

            if (cs->pieza == PUMA) {
                pumas++;

                if (t->puma.fila != f || t->puma.col != c)
                    return false;
            } else {
                perros++;

                int id = cs->perro;

                if (id < 0 || id >= NUM_PERROS || !t->perro_vivo[id])
                    return false;

                if (t->perros[id].fila != f || t->perros[id].col != c)
                    return false;
            }
        }
    }

    return pumas == 1 && perros == NUM_PERROS - t->capturados;
}

// Toma una instantánea consistente del tablero y revisa la victoria.
static void revisar_tablero(Partida *p) {
    partida_bloquear_todo(p);

    if (!p->terminada) {
        fprintf(p->registro, "Tablero tras %d movimientos:\n",
                p->movimientos);

        tablero_imprimir(&p->tablero, p->registro);

        Pos destinos[MAX_VECINOS];

        if (!estado_consistente(&p->tablero)) {
            fprintf(p->registro,
                    "Estado inconsistente, se aborta la partida\n");

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
    d->abandono = false;
    d->interrumpida = false;

    Partida p;

    if (!partida_inicializar(&p, d->id_mesa, d->id_partida, d->config))
        return NULL;

    // Registra la partida activa en el servidor.
    pthread_mutex_lock(d->mutex_servidor);

    *d->registro = &p;
    bool cerrando = *d->cerrando;

    pthread_mutex_unlock(d->mutex_servidor);

    if (cerrando)
        partida_interrumpir(&p);

    printf("[mesa %d] Comienza la partida %d (log en partida_%d.log)\n",
           d->id_mesa, d->id_partida, d->id_partida);

    fprintf(p.registro, "Partida %d en la mesa %d\n",
            d->id_partida, d->id_mesa);

    revisar_tablero(&p);

    int creados = partida_crear_fichas(&p);

    if (creados < NUM_FICHAS) {
        pthread_mutex_lock(&p.mutex_estado);

        partida_anunciar_fin(&p, SIN_GANADOR);

        pthread_mutex_unlock(&p.mutex_estado);
    }

    // El juez espera eventos de las fichas.
    int revisados = 0;

    while (true) {
        pthread_mutex_lock(&p.mutex_estado);

        while (!p.terminada && p.eventos == revisados) {
            pthread_cond_wait(&p.cond_juez, &p.mutex_estado);
        }

        if (!p.terminada) {
            int falla = buscar_falla(&p);

            if (falla >= 0)
                declarar_abandono(&p, falla);
        }

        bool fin = p.terminada;
        revisados = p.eventos;

        pthread_mutex_unlock(&p.mutex_estado);

        if (fin)
            break;

        revisar_tablero(&p);
    }

    // Espera a todas las fichas antes de liberar la mesa.
    partida_esperar_fichas(&p, creados);

    pthread_mutex_lock(d->mutex_servidor);

    *d->registro = NULL;

    pthread_mutex_unlock(d->mutex_servidor);

    const char *texto = juez_texto_resultado(
        p.resultado, p.abandono, p.interrumpida
    );

    fprintf(p.registro,
            "Fin de la partida tras %d movimientos: %s\n",
            p.movimientos, texto);

    printf("[mesa %d] Fin de la partida %d tras %d movimientos: %s\n",
           d->id_mesa, d->id_partida, p.movimientos, texto);

    // Comprueba que ningún mutex haya quedado retenido.
    int retenidos = partida_mutex_retenidos(&p);

    fprintf(p.registro, "Mutex retenidos al cerrar la mesa: %d\n",
            retenidos);

    if (retenidos > 0) {
        fprintf(stderr,
                "[mesa %d] Quedaron %d mutex tomados en la partida %d\n",
                d->id_mesa, retenidos, d->id_partida);
    }

    d->resultado = p.resultado;
    d->abandono = p.abandono;
    d->interrumpida = p.interrumpida;

    partida_destruir(&p);

    return NULL;
}
