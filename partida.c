#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "partida.h"

// Calcula las casillas a distancia menor o igual a radio desde p, ordenadas de menor a mayor indice
static int calcular_zona(Pos p, int radio, int zona[FILAS * COLUMNAS]) {
    bool en_zona[FILAS][COLUMNAS];
    memset(en_zona, 0, sizeof(en_zona));
    en_zona[p.fila][p.col] = true;

    for (int r = 0; r < radio; r++) {
        bool nueva[FILAS][COLUMNAS];
        memcpy(nueva, en_zona, sizeof(nueva));
        for (int a = 0; a < FILAS * COLUMNAS; a++) {
            Pos pa = {a / COLUMNAS, a % COLUMNAS};
            if (!en_zona[pa.fila][pa.col]) continue;
            for (int b = 0; b < FILAS * COLUMNAS; b++) {
                Pos pb = {b / COLUMNAS, b % COLUMNAS};
                if (tablero_adyacentes(pa, pb)) nueva[pb.fila][pb.col] = true;
            }
        }
        memcpy(en_zona, nueva, sizeof(en_zona));
    }

    // Se recorre la matriz por filas, por lo que los indices quedan en orden ascendente
    int n = 0;
    for (int f = 0; f < FILAS; f++)
        for (int c = 0; c < COLUMNAS; c++)
            if (en_zona[f][c]) zona[n++] = f * COLUMNAS + c;
    return n;
}

// Bloquea las casillas de la zona en orden ascendente para evitar deadlock
static void bloquear_zona(Partida *p, const int zona[], int n) {
    for (int i = 0; i < n; i++)
        pthread_mutex_lock(&p->mutex_casilla[zona[i] / COLUMNAS][zona[i] % COLUMNAS]);
}

// Libera las casillas de la zona en orden inverso
static void desbloquear_zona(Partida *p, const int zona[], int n) {
    for (int i = n - 1; i >= 0; i--)
        pthread_mutex_unlock(&p->mutex_casilla[zona[i] / COLUMNAS][zona[i] % COLUMNAS]);
}

void partida_anunciar_fin(Partida *p, Resultado r) {
    if (!p->terminada) {
        p->terminada = true;
        p->resultado = r;
    }
    pthread_cond_broadcast(&p->cond_fin);
    pthread_cond_signal(&p->cond_juez);
}

// Interrumpir ?? Palimpacta tocadora

void partida_interrumpir(Partida *p) {
    pthread_mutex_lock(&p->mutex_estado);

    if (!p->terminada) {
	p->interrumpida = true;
	fprintf(p->registro, "Partida ABORTADA por la maria (servidor)\n");
}

     partida_anunciar_fin(p, SIN_GANADOR);

     pthread_mutex_unlock(&p->mutex_estado);
}


// Avisa al juez que hay algo nuevo que revisar. Se llama con mutex_estado tomado
static void avisar_juez(Partida *p) {
    p->eventos++;
    pthread_cond_signal(&p->cond_juez);
}

typedef struct {
    Partida  *partida;
    const int *zona;
    int n;
} ZonaTomada;

static void limpiar_zona(void *arg) {
    ZonaTomada *z = arg;
    desbloquear_zona(z->partida, z->zona, z->n);
}

// Ñ
static void limpiar_estado(void *arg) {
    Partida *p = arg;
    pthread_mutex_unlock(&p->mutex_estado);
}

// Marca la ficha como terminada y aviza al Zuniga que puso mal el nombre (Juez)
static void marcar_fin_ficha(void *arg) {
    Ficha *f = arg;
    Partida *p = f->partida;

    pthread_mutex_lock(&p->mutex_estado);
    f->activa = false;
    avisar_juez(p);
    pthread_mutex_unlock(&p->mutex_estado);
}

// Simula a una persona jugando ping pong (fallo de fracasadito)

static void simular_falla(Ficha *f, const char *nombre) {
    fprintf(f->partida->registro,
	"Falla interna de %s: su hilo termina abruptamente\n",
	nombre);

    pthread_exit(NULL);
}


// Indica si las fichas pueden seguir moviendose. Se llama con mutex_estado tomado
static bool partida_activa(const Partida *p) {
    return !p->terminada && p->tablero.capturados < CAPTURAS_PARA_GANAR;
}

// Espera un tiempo aleatorio entre retardo_min y retardo_max sin espera activa. Retorna false si la partida termino
static bool esperar_retardo(Partida *p, unsigned int *semilla) {
    const Config *cfg = p->config;
    unsigned int rango = (unsigned int)(cfg->retardo_max - cfg->retardo_min) + 1u;
    int ms = cfg->retardo_min + (int)((unsigned int)rand_r(semilla) % rango);

    struct timespec limite;
    clock_gettime(CLOCK_REALTIME, &limite);
    limite.tv_sec += ms / 1000;
    limite.tv_nsec += (long)(ms % 1000) * 1000000L;
    if (limite.tv_nsec >= 1000000000L) {
        limite.tv_sec++;
        limite.tv_nsec -= 1000000000L;
    }

    // pthread_cond_timedwait libera el mutex mientras espera y retorna antes si se anuncia el fin de la partida
    pthread_mutex_lock(&p->mutex_estado);
    int r = 0;
    while (!p->terminada && r != ETIMEDOUT)
        r = pthread_cond_timedwait(&p->cond_fin, &p->mutex_estado, &limite);
    bool sigue = !p->terminada;
    pthread_mutex_unlock(&p->mutex_estado);
    return sigue;
}

// Registra un movimiento en el log y avisa al juez. Se llama con las casillas y mutex_estado tomados, asi el orden del log es el orden real
static void registrar_movimiento(Partida *p, const char *ficha, Pos desde, Pos hacia, int capturado) {
    p->movimientos++;
    fprintf(p->registro, "%d. %s: (%d,%d) -> (%d,%d)", p->movimientos, ficha,
            desde.fila, desde.col, hacia.fila, hacia.col);
    if (capturado >= 0) {
        fprintf(p->registro, "  captura al perro %d", capturado);
        printf("[mesa %d] El puma captura al perro %d (%d de %d)\n", p->id_mesa, capturado,
               p->tablero.capturados, CAPTURAS_PARA_GANAR);
    }
    fprintf(p->registro, "\n");
    avisar_juez(p);
}

// Registra un movimiento que no cumple las reglas. Se llama con las casillas y mutex_estado tomados
static void registrar_rechazo(Partida *p, const char *ficha, Pos desde, Pos hacia) {
    fprintf(p->registro, "Movimiento rechazado de %s: (%d,%d) -> (%d,%d)\n", ficha,
            desde.fila, desde.col, hacia.fila, hacia.col);
}

static void *hilo_perro(void *arg) {
    Ficha *f = arg;
    Partida *p = f->partida;
    char nombre[16];
    snprintf(nombre, sizeof(nombre), "perro %d", f->id);

    pthread_cleanup_push(marcar_fin_ficha, f);

    while (esperar_retardo(p, &f->semilla)) {
	f->ciclos++;
        // Solo este hilo modifica la posicion de su perro, por eso puede leerla antes de bloquear
        Pos origen = p->tablero.perros[f->id];
        int zona[FILAS * COLUMNAS];
        int n = calcular_zona(origen, 1, zona);

	ZonaTomada tomada = {p, zona, n};
	bool activa;
	bool vivo;

        bloquear_zona(p, zona, n);
	pthread_cleanup_push(limpiar_zona, &tomada);

        pthread_mutex_lock(&p->mutex_estado);
	pthread_cleanup_push(limpiar_estado, p);

	if (f->ciclos == f->falla_en)
	   simular_falla(f, nombre);

        activa = partida_activa(p);
        vivo = p->tablero.perro_vivo[f->id];
        if (activa && vivo) {
            Pos destinos[MAX_VECINOS];
            int k = tablero_movimientos_perro(&p->tablero, f->id, destinos);
            if (k > 0) {
                Pos destino = destinos[rand_r(&f->semilla) % k];
                if (tablero_mover_perro(&p->tablero, f->id, destino))
                    registrar_movimiento(p, nombre, origen, destino, -1);
                else
                    registrar_rechazo(p, nombre, origen, destino);
            }
        }
        pthread_cleanup_pop(1);
        pthread_cleanup_pop(1);

        if (!activa || !vivo) break;   // termino la partida o el perro fue capturado
    }
    pthread_cleanup_pop(1); // termina la ficha
    return NULL;
}

static void *hilo_puma(void *arg) {
    Ficha *f = arg;
    Partida *p = f->partida;

    pthread_cleanup_push(marcar_fin_ficha, f);

    while (esperar_retardo(p, &f->semilla)) {
	f->ciclos++;

	// Solo este hilo modifica la posicion del puma, por eso puede leerla antes de bloquear
        Pos origen = p->tablero.puma;
        int zona[FILAS * COLUMNAS];
        int n = calcular_zona(origen, 2, zona);   // radio 2 para incluir los destinos de captura

	ZonaTomada tomada = {p, zona, n};
	bool activa;

        bloquear_zona(p, zona, n);
	pthread_cleanup_push(limpiar_zona, &tomada);

        pthread_mutex_lock(&p->mutex_estado);
	pthread_cleanup_push(limpiar_estado, p);

	if (f->ciclos == f->falla_en)
	    simular_falla(f, "puma");

	activa = partida_activa(p);
        if (activa) {
            Pos destinos[MAX_VECINOS], capturas[MAX_VECINOS];
            int k = tablero_movimientos_puma(&p->tablero, destinos);
            int c = 0;
            for (int i = 0; i < k; i++)
                if (!tablero_adyacentes(origen, destinos[i])) capturas[c++] = destinos[i];

            if (k == 0) {
                avisar_juez(p);   // el puma no puede moverse y el juez debe revisar si quedo acorralado
            } else {
                // El puma prefiere capturar cuando puede
                Pos destino = (c > 0) ? capturas[rand_r(&f->semilla) % c] : destinos[rand_r(&f->semilla) % k];
                int capturado;
                if (tablero_mover_puma(&p->tablero, destino, &capturado))
                    registrar_movimiento(p, "puma", origen, destino, capturado);
                else
                    registrar_rechazo(p, "puma", origen, destino);
            }
        }
        pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);

        if (!activa) break;
    }
    pthread_cleanup_pop(1);
    return NULL;
}

bool partida_inicializar(Partida *p, int id_mesa, int id_partida, const Config *cfg) {
    p->id_mesa = id_mesa;
    p->id_partida = id_partida;
    p->config = cfg;
    p->terminada = false;
    p->resultado = SIN_GANADOR;
    p->interrumpida = false;
    p->abandono = false;
    p->movimientos = 0;
    p->eventos = 0;
    if (!tablero_inicializar(&p->tablero, cfg->puma, cfg->perros)) {
        fprintf(stderr, "[mesa %d] Disposicion inicial invalida\n", id_mesa);
        return false;
    }

    char nombre[64];
    snprintf(nombre, sizeof(nombre), "partida_%d.log", id_partida);
    p->registro = fopen(nombre, "w");
    if (p->registro == NULL) {
        fprintf(stderr, "[mesa %d] No se pudo crear %s: %s\n", id_mesa, nombre, strerror(errno));
        return false;
    }

    for (int f = 0; f < FILAS; f++)
        for (int c = 0; c < COLUMNAS; c++)
            pthread_mutex_init(&p->mutex_casilla[f][c], NULL);
    pthread_mutex_init(&p->mutex_estado, NULL);
    pthread_cond_init(&p->cond_fin, NULL);
    pthread_cond_init(&p->cond_juez, NULL);
    return true;
}

void partida_destruir(Partida *p) {
    fclose(p->registro);
    for (int f = 0; f < FILAS; f++)
        for (int c = 0; c < COLUMNAS; c++)
            pthread_mutex_destroy(&p->mutex_casilla[f][c]);
    pthread_mutex_destroy(&p->mutex_estado);
    pthread_cond_destroy(&p->cond_fin);
    pthread_cond_destroy(&p->cond_juez);
}

int partida_crear_fichas(Partida *p) {

    unsigned int semilla_fallo =
	p->config->semilla + 7919u * (unsigned int)p->id_partida;

    int ficha_falla = -1;
    int ciclo_falla = -1;

    if ((int)(rand_r(&semilla_fallo) % 100u) < p->config->fok_israel) {
	ficha_falla = (int)(rand_r(&semilla_fallo) % NUM_FICHAS);
	ciclo_falla = 1 + (int)(rand_r(&semilla_fallo) % 20u);
}

    // La ficha 0 es el puma y las fichas 1 a 12 son los perros
    int creados = 0;
    for (int i = 0; i < NUM_FICHAS; i++) {
        p->fichas[i].partida = p;
        p->fichas[i].id = (i == 0) ? ID_PUMA : i - 1;
        p->fichas[i].semilla = p->config->semilla + 1000u * (unsigned int)p->id_partida + (unsigned int)i;
        p->fichas[i].ciclos = 0;
	p->fichas[i].falla_en = (i == ficha_falla) ? ciclo_falla : -1;
	p->fichas[i].activa = true;
	void *(*rutina)(void *) = (i == 0) ? hilo_puma : hilo_perro;
        int err = pthread_create(&p->hilos[i], NULL, rutina, &p->fichas[i]);
        if (err != 0) {
            fprintf(stderr, "[mesa %d] No se pudo crear un hilo: %s\n", p->id_mesa, strerror(err));
            break;
        }
        creados++;
    }
    return creados;
}

void partida_esperar_fichas(Partida *p, int creados) {
    for (int i = 0; i < creados; i++)
        pthread_join(p->hilos[i], NULL);
}

int partida_mutex_retenidos(Partida *p) {
	int retenidos = 0;

	for (int f = 0; f < FILAS; f++) {
	    for (int c = 0; c < COLUMNAS; c++) {
		if (pthread_mutex_trylock(&p->mutex_casilla[f][c]) == 0)
		   (pthread_mutex_unlock(&p->mutex_casilla[f][c]));
		else
		    retenidos++;
	    }
	}
	if (pthread_mutex_trylock(&p->mutex_estado) == 0)
	    pthread_mutex_unlock(&p->mutex_estado);
	else
	    retenidos++;

	return retenidos;
}

void partida_bloquear_todo(Partida *p) {
    for (int f = 0; f < FILAS; f++)
        for (int c = 0; c < COLUMNAS; c++)
            pthread_mutex_lock(&p->mutex_casilla[f][c]);
    pthread_mutex_lock(&p->mutex_estado);
}

void partida_desbloquear_todo(Partida *p) {
    pthread_mutex_unlock(&p->mutex_estado);
    for (int f = FILAS - 1; f >= 0; f--)
        for (int c = COLUMNAS - 1; c >= 0; c--)
            pthread_mutex_unlock(&p->mutex_casilla[f][c]);
}

int partida_retardo_aleatorio(const Config *cfg, unsigned int *semilla) {
    unsigned int rango =
        (unsigned int)(cfg->retardo_max - cfg->retardo_min) + 1u;

    return cfg->retardo_min +
           (int)((unsigned int)rand_r(semilla) % rango);
}

void partida_tiempo_limite(int ms, struct timespec *limite) {
    clock_gettime(CLOCK_REALTIME, limite);

    limite->tv_sec += ms / 1000;
    limite->tv_nsec += (long)(ms % 1000) * 1000000L;

    if (limite->tv_nsec >= 1000000000L) {
        limite->tv_sec++;
        limite->tv_nsec -= 1000000000L;
    }
}
