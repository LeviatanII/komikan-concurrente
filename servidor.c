#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "servidor.h"
#include "juez.h"

bool servidor_iniciar(Servidor *s, int num_jugadores, int num_mesas,
                      const Config *cfg) {
    memset(s, 0, sizeof(*s));
    s->config = cfg;
    s->num_jugadores = num_jugadores;
    s->num_mesas = num_mesas;
    s->esperando = -1;

    s->mesa_ocupada = calloc((size_t)num_mesas, sizeof(bool));
    s->partida_en_mesa = calloc((size_t)num_mesas, sizeof(Partida *));
    s->jugadores = calloc((size_t)num_jugadores, sizeof(Jugador));
    s->hilos = calloc((size_t)num_jugadores, sizeof(pthread_t));

    bool memoria_ok = s->mesa_ocupada != NULL &&
                      s->partida_en_mesa != NULL &&
                      s->jugadores != NULL &&
                      s->hilos != NULL;

    if (!memoria_ok) {
        fprintf(stderr, "[servidor] Memoria insuficiente\n");
        goto error_memoria;
    }

    if (sem_init(&s->mesas_libres, 0, (unsigned int)num_mesas) != 0) {
        fprintf(stderr, "[servidor] Error al crear semaforo: %s\n",
                strerror(errno));
        goto error_memoria;
    }

    if (pthread_mutex_init(&s->mutex, NULL) != 0) {
        sem_destroy(&s->mesas_libres);
        goto error_memoria;
    }

    if (pthread_cond_init(&s->cond, NULL) != 0) {
        pthread_mutex_destroy(&s->mutex);
        sem_destroy(&s->mesas_libres);
        goto error_memoria;
    }

    return true;

error_memoria:
    free(s->mesa_ocupada);
    free(s->partida_en_mesa);
    free(s->jugadores);
    free(s->hilos);
    return false;
}

void servidor_destruir(Servidor *s) {
    sem_destroy(&s->mesas_libres);
    pthread_mutex_destroy(&s->mutex);
    pthread_cond_destroy(&s->cond);

    free(s->mesa_ocupada);
    free(s->partida_en_mesa);
    free(s->jugadores);
    free(s->hilos);
}

void servidor_interrumpir(Servidor *s) {
    pthread_mutex_lock(&s->mutex);

    if (!s->cerrando) {
        s->cerrando = true;

        printf("[servidor] Ctrl+C recibido: cerrando torneo\n");

        pthread_cond_broadcast(&s->cond);

        for (int m = 0; m < s->num_mesas; m++) {
            if (s->partida_en_mesa[m] != NULL) {
                partida_interrumpir(s->partida_en_mesa[m]);
            }
        }
    }

    pthread_mutex_unlock(&s->mutex);
}

static void *hilo_senales(void *arg) {
    Servidor *s = arg;

    sigset_t conjunto;
    sigemptyset(&conjunto);
    sigaddset(&conjunto, SIGINT);

    int senal;

    while (sigwait(&conjunto, &senal) == 0) {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        servidor_interrumpir(s);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }

    return NULL;
}

static bool esperar_llegada(Servidor *s, Jugador *j) {
    struct timespec limite;

    partida_tiempo_limite(
        partida_retardo_aleatorio(s->config, &j->semilla),
        &limite
    );

    pthread_mutex_lock(&s->mutex);

    int r = 0;

    while (!s->cerrando && r != ETIMEDOUT) {
        r = pthread_cond_timedwait(&s->cond, &s->mutex, &limite);
    }

    bool llega = !s->cerrando;

    pthread_mutex_unlock(&s->mutex);

    return llega;
}

static void terminar_pareja(Servidor *s, Jugador *puma,
                            Jugador *perros) {
    puma->terminado = true;
    perros->terminado = true;

    pthread_cond_broadcast(&s->cond);
}

static void jugar_partida(Servidor *s, Jugador *puma,
                          Jugador *perros) {

    if (sem_trywait(&s->mesas_libres) != 0) {
        printf("[servidor] Jugadores %d y %d esperando mesa libre\n",
               puma->id, perros->id);

        while (sem_wait(&s->mesas_libres) != 0) {
            if (errno != EINTR) {
                fprintf(stderr, "[servidor] Error esperando mesa\n");
                pthread_mutex_lock(&s->mutex);
                terminar_pareja(s, puma, perros);
                pthread_mutex_unlock(&s->mutex);
                return;
            }
        }
    }

    pthread_mutex_lock(&s->mutex);

    if (s->cerrando) {
        printf("[servidor] Pareja %d-%d retirada por cierre\n",
               puma->id, perros->id);

        terminar_pareja(s, puma, perros);

        pthread_mutex_unlock(&s->mutex);
        sem_post(&s->mesas_libres);
        return;
    }

    int mesa = 0;

    while (mesa < s->num_mesas && s->mesa_ocupada[mesa]) {
        mesa++;
    }

    if (mesa == s->num_mesas) {
        terminar_pareja(s, puma, perros);
        pthread_mutex_unlock(&s->mutex);
        sem_post(&s->mesas_libres);
        return;
    }

    s->mesa_ocupada[mesa] = true;
    s->mesas_activas++;

    if (s->mesas_activas > s->max_mesas_activas) {
        s->max_mesas_activas = s->mesas_activas;
    }

    int id_partida = ++s->partidas;

    puma->id_partida = id_partida;
    perros->id_partida = id_partida;

    printf("[servidor] Mesa %d: partida %d. Puma %d contra perros %d\n",
           mesa, id_partida, puma->id, perros->id);

    pthread_mutex_unlock(&s->mutex);

    DatosJuez juez = {
        mesa,
        id_partida,
        s->config,
        &s->mutex,
        &s->partida_en_mesa[mesa],
        &s->cerrando,
        SIN_GANADOR,
        false,
        false
    };

    pthread_t hilo;
    int err = pthread_create(&hilo, NULL, hilo_juez, &juez);

    if (err != 0) {
        fprintf(stderr, "[servidor] Error creando juez: %s\n",
                strerror(err));
    } else {
        pthread_join(hilo, NULL);
    }

    pthread_mutex_lock(&s->mutex);

    s->mesa_ocupada[mesa] = false;
    s->mesas_activas--;

    if (juez.interrumpida) {
        s->interrumpidas++;
    } else if (juez.resultado == GANA_PUMA) {
        s->victorias_puma++;
    } else if (juez.resultado == GANA_PERROS) {
        s->victorias_perros++;
    } else {
        s->sin_ganador++;
    }

    if (juez.abandono) {
        s->abandonos++;
    }

    puma->resultado = juez.resultado;
    perros->resultado = juez.resultado;

    printf("[servidor] Mesa %d liberada\n", mesa);

    terminar_pareja(s, puma, perros);

    pthread_mutex_unlock(&s->mutex);

    sem_post(&s->mesas_libres);
}

static void *hilo_jugador(void *arg) {
    Jugador *j = arg;
    Servidor *s = j->servidor;

    if (!esperar_llegada(s, j)) {
        return NULL;
    }

    pthread_mutex_lock(&s->mutex);

    if (s->cerrando) {
        pthread_mutex_unlock(&s->mutex);
        return NULL;
    }

    s->llegados++;

    printf("[servidor] Llega jugador %d\n", j->id);

    if (s->esperando >= 0) {
        Jugador *otro = &s->jugadores[s->esperando];

        s->esperando = -1;

        otro->emparejado = true;
        otro->rol = ROL_PUMA;
        otro->companero = j->id;

        j->emparejado = true;
        j->rol = ROL_PERROS;
        j->companero = otro->id;

        printf("[servidor] Pareja formada: Puma %d contra perros %d\n",
               otro->id, j->id);

        pthread_cond_broadcast(&s->cond);
        pthread_mutex_unlock(&s->mutex);

        jugar_partida(s, otro, j);

        return NULL;
    }

    s->esperando = j->id;

    while (!j->emparejado &&
           s->llegados < s->num_jugadores &&
           !s->cerrando) {
        pthread_cond_wait(&s->cond, &s->mutex);
    }

    if (!j->emparejado) {
        s->esperando = -1;
        s->sin_pareja++;
        j->terminado = true;

        printf("[servidor] Jugador %d queda sin pareja\n", j->id);
    }

    while (!j->terminado) {
        pthread_cond_wait(&s->cond, &s->mutex);
    }

    pthread_mutex_unlock(&s->mutex);

    return NULL;
}

void servidor_ejecutar(Servidor *s) {
    sigset_t conjunto;

    sigemptyset(&conjunto);
    sigaddset(&conjunto, SIGINT);

    pthread_sigmask(SIG_BLOCK, &conjunto, NULL);

    pthread_t senales;

    int err_senales = pthread_create(
        &senales, NULL, hilo_senales, s
    );

    if (err_senales != 0) {
        fprintf(stderr, "[servidor] Error creando hilo de señales\n");
    }

    int creados = 0;

    for (int i = 0; i < s->num_jugadores; i++) {
        Jugador *j = &s->jugadores[i];

        j->servidor = s;
        j->id = i;
        j->semilla = s->config->semilla + 100000u + (unsigned int)i;
        j->companero = -1;
        j->id_partida = -1;
        j->resultado = SIN_GANADOR;

        int err = pthread_create(
            &s->hilos[i], NULL, hilo_jugador, j
        );

        if (err != 0) {
            fprintf(stderr, "[servidor] Error creando jugador %d: %s\n",
                    i, strerror(err));

            pthread_mutex_lock(&s->mutex);
            s->num_jugadores = i;
            pthread_cond_broadcast(&s->cond);
            pthread_mutex_unlock(&s->mutex);

            break;
        }

        creados++;
    }

    for (int i = 0; i < creados; i++) {
        pthread_join(s->hilos[i], NULL);
    }

    if (err_senales == 0) {
        pthread_cancel(senales);
        pthread_join(senales, NULL);
    }
}

void servidor_mostrar_resumen(const Servidor *s) {
    if (s->cerrando) {
        printf("[servidor] Torneo interrumpido. Jugadores que no llegaron: %d\n",
               s->num_jugadores - s->llegados);
    }

    printf("[servidor] Partidas jugadas: %d\n", s->partidas);
    printf("[servidor] Victorias Puma: %d\n", s->victorias_puma);
    printf("[servidor] Victorias perros: %d\n", s->victorias_perros);
    printf("[servidor] Partidas interrumpidas: %d\n", s->interrumpidas);
    printf("[servidor] Partidas sin ganador: %d\n", s->sin_ganador);
    printf("[servidor] Abandonos: %d\n", s->abandonos);
    printf("[servidor] Jugadores sin pareja: %d\n", s->sin_pareja);

    printf("[servidor] Maximo de mesas activas: %d de %d\n",
           s->max_mesas_activas, s->num_mesas);
}
