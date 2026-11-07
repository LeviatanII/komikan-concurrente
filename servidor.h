#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#include "config.h"
#include "partida.h"

typedef enum {
	ROL_PUMA,
	ROL_PERROS
}   Rol;

typedef struct Servidor Servidor;

typedef struct {
	Servidor *servidor;
	int id;
	unsigned int semilla;
	bool emparejado;
	bool terminado;
	Rol rol;
	int companero;
	int id_partida;
	Resultado resultado;
}   Jugador;

struct Servidor {

	const Config *config;
	int num_jugadores;
	int num_mesas;

	sem_t mesas_libres;
	pthread_mutex_t mutex;
	pthread_cond_t cond;

	bool cerrando;
	Partida **partida_en_mesa;

	int llegados;
	int esperando;
	bool *mesa_ocupada;
	int mesas_activas;
	int max_mesas_activas;

	int partidas;
	int victorias_puma;
	int victorias_perros;
	int sin_ganador;
	int abandonos;
	int interrumpidas;
	int sin_pareja;

	Jugador *jugadores;
	pthread_t *hilos;
};

bool servidor_iniciar(Servidor *s, int num_jugadores, int num_mesas, const Config *cfg);

void servidor_ejecutar(Servidor *s);
void servidor_interrumpir(Servidor *s);
void servidor_mostrar_resumen(const Servidor *s);
void servidor_destruir(Servidor *s);

#endif
