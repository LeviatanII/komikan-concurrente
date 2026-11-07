
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "config.h"
#include "servidor.h"

#define MAX_JUGADORES 1000
#define MAX_MESAS 100

#define RET_OK 0
#define RET_ERROR 1
#define RET_ARGUMENTOS 2

static void uso(const char *prog) {
    fprintf(stderr, "Uso: %s N M [config.txt]\n", prog);
    fprintf(stderr, "  N: numero de jugadores (entre 2 y %d)\n",
            MAX_JUGADORES);
    fprintf(stderr, "  M: numero maximo de mesas (entre 1 y %d)\n",
            MAX_MESAS);
}

static long parsear_entero(const char *s) {
    char *fin;

    errno = 0;
    long v = strtol(s, &fin, 10);

    if (errno != 0 || *fin != '\0' || v <= 0)
        return -1;

    return v;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        uso(argv[0]);
        return RET_ARGUMENTOS;
    }

    long n = parsear_entero(argv[1]);
    long m = parsear_entero(argv[2]);

    if (n < 2 || m < 1 ||
        n > MAX_JUGADORES || m > MAX_MESAS) {
        uso(argv[0]);
        return RET_ARGUMENTOS;
    }

    const char *ruta_config = (argc == 4) ? argv[3] : NULL;

    printf("[torneo] N=%ld jugadores, M=%ld mesas, config=%s\n",
           n, m, ruta_config ? ruta_config : "(valores por defecto)");

    if (n % 2 != 0) {
        printf("[torneo] Aviso: N es impar, un jugador quedara sin pareja.\n");
    }

    Config config;
    config_defecto(&config);

    if (ruta_config != NULL && !config_cargar(ruta_config, &config))
        return RET_ARGUMENTOS;

    config_mostrar(&config);

    Servidor servidor;

    if (!servidor_iniciar(&servidor, (int)n, (int)m, &config))
        return RET_ERROR;

    servidor_ejecutar(&servidor);
    servidor_mostrar_resumen(&servidor);

    bool hubo_errores = servidor.sin_ganador > 0 || servidor.cerrando;

    servidor_destruir(&servidor);

    if (hubo_errores)
        return RET_ERROR;

    return RET_OK;
}
