#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "tablero.h"
#include "config.h"
#include "partida.h"

// Codigos de retorno del programa
#define RET_OK         0   // ejecucion correcta
#define RET_ERROR      1   // error o interrupcion durante la ejecucion
#define RET_ARGUMENTOS 2   // argumentos o configuracion invalidos

// Muestra la forma correcta de ejecutar el programa
static void uso(const char *prog) {
    fprintf(stderr, "Uso: %s N M [config.txt]\n", prog);
    fprintf(stderr, "  N: numero total de jugadores (>= 2)\n");
    fprintf(stderr, "  M: numero maximo de mesas concurrentes (>= 1)\n");
}

// Convierte un string a entero positivo, retorna -1 si es invalido
static long parsear_entero(const char *s) {
    char *fin;
    errno = 0;
    long v = strtol(s, &fin, 10);
    if (errno != 0 || *fin != '\0' || v <= 0) return -1;
    return v;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        uso(argv[0]);
        return RET_ARGUMENTOS;
    }
    long n = parsear_entero(argv[1]);
    long m = parsear_entero(argv[2]);
    if (n < 2 || m < 1) {
        uso(argv[0]);
        return RET_ARGUMENTOS;
    }
    const char *ruta_config = (argc == 4) ? argv[3] : NULL;

    printf("[torneo] N=%ld jugadores, M=%ld mesas, config=%s\n",
           n, m, ruta_config ? ruta_config : "(valores por defecto)");
    if (n % 2 != 0)
        printf("[torneo] Aviso: N es impar, un jugador quedara sin pareja.\n");

    // Se cargan los valores por defecto y luego el archivo, si se indico
    Config config;
    config_defecto(&config);
    if (ruta_config != NULL && !config_cargar(ruta_config, &config))
        return RET_ARGUMENTOS;
    config_mostrar(&config);

    // Por ahora se juega una sola partida en la mesa 0
    Resultado resultado = partida_jugar(0, &config);
    if (resultado == SIN_GANADOR)
        return RET_ERROR;

    return RET_OK;
}
