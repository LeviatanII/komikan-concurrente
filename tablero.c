#include "tablero.h"

// Conexiones del triangulo. La cuadricula de 5 x 5 solo tiene conexiones horizontales y verticales, que se calculan aparte
static const Pos ARISTAS_TRIANGULO[][2] = {
    {{0, 2}, {1, 1}},   // vertice - base izquierda
    {{0, 2}, {1, 3}},   // vertice - base derecha
    {{0, 2}, {2, 2}},   // vertice - centro de la primera fila
    {{1, 1}, {1, 3}},   // base izquierda - base derecha
    {{1, 1}, {2, 0}},   // base izquierda - esquina izquierda
    {{1, 1}, {2, 1}},
    {{1, 3}, {2, 3}},
    {{1, 3}, {2, 4}},   // base derecha - esquina derecha
};
#define NUM_ARISTAS_TRIANGULO \
    (int)(sizeof(ARISTAS_TRIANGULO) / sizeof(ARISTAS_TRIANGULO[0]))

static int signo(int x) {
    return (x > 0) - (x < 0);
}

static bool pos_iguales(Pos a, Pos b) {
    return a.fila == b.fila && a.col == b.col;
}

bool tablero_es_nodo(Pos p) {
    if (p.fila < 0 || p.fila >= FILAS || p.col < 0 || p.col >= COLUMNAS)
        return false;
    if (p.fila == 0) return p.col == 2;
    if (p.fila == 1) return p.col == 1 || p.col == 3;
    return true;
}

bool tablero_adyacentes(Pos a, Pos b) {
    if (!tablero_es_nodo(a) || !tablero_es_nodo(b) || pos_iguales(a, b))
        return false;

    // Cuadricula: vecinos horizontales o verticales a distancia 1
    if (a.fila >= 2 && b.fila >= 2) {
        int df = a.fila - b.fila, dc = a.col - b.col;
        return (df == 0 && (dc == 1 || dc == -1)) ||
               (dc == 0 && (df == 1 || df == -1));
    }

    // Triangulo: se revisa la lista de conexiones en ambos sentidos
    for (int i = 0; i < NUM_ARISTAS_TRIANGULO; i++) {
        Pos x = ARISTAS_TRIANGULO[i][0], y = ARISTAS_TRIANGULO[i][1];
        if ((pos_iguales(a, x) && pos_iguales(b, y)) ||
            (pos_iguales(a, y) && pos_iguales(b, x)))
            return true;
    }
    return false;
}

// Un salto es valido si desde-sobre y sobre-hacia estan conectados y ambos tramos siguen la misma direccion (misma línea recta)
bool tablero_es_salto(Pos desde, Pos sobre, Pos hacia) {
    if (!tablero_adyacentes(desde, sobre) || !tablero_adyacentes(sobre, hacia))
        return false;
    return signo(sobre.fila - desde.fila) == signo(hacia.fila - sobre.fila) &&
           signo(sobre.col - desde.col) == signo(hacia.col - sobre.col);
}

void tablero_disposicion_defecto(Pos *puma, Pos perros[NUM_PERROS]) {
    *puma = (Pos){0, 2};
    int k = 0;
    for (int f = 5; f <= 6; f++)
        for (int c = 0; c < COLUMNAS; c++)
            perros[k++] = (Pos){f, c};
    perros[k++] = (Pos){4, 0};
    perros[k++] = (Pos){4, 4};
}

bool tablero_inicializar(Tablero *t, Pos puma, const Pos perros[NUM_PERROS]) {
    for (int f = 0; f < FILAS; f++) {
        for (int c = 0; c < COLUMNAS; c++) {
            t->casilla[f][c].existe = tablero_es_nodo((Pos){f, c});
            t->casilla[f][c].pieza = VACIA;
            t->casilla[f][c].perro = -1;
        }
    }
    t->capturados = 0;

    if (!tablero_es_nodo(puma)) return false;
    t->puma = puma;
    t->casilla[puma.fila][puma.col].pieza = PUMA;

    for (int i = 0; i < NUM_PERROS; i++) {
        Pos p = perros[i];
        if (!tablero_es_nodo(p) || t->casilla[p.fila][p.col].pieza != VACIA)
            return false;
        t->perros[i] = p;
        t->perro_vivo[i] = true;
        t->casilla[p.fila][p.col].pieza = PERRO;
        t->casilla[p.fila][p.col].perro = i;
    }
    return true;
}

static Pieza contenido(const Tablero *t, Pos p) {
    return t->casilla[p.fila][p.col].pieza;
}

// Recorre todos los nodos y guarda los que son adyacentes a p
static int vecinos(Pos p, Pos salida[MAX_VECINOS]) {
    int n = 0;
    for (int f = 0; f < FILAS; f++) {
        for (int c = 0; c < COLUMNAS; c++) {
            Pos q = {f, c};
            if (n < MAX_VECINOS && tablero_adyacentes(p, q))
                salida[n++] = q;
        }
    }
    return n;
}

// Busca un perro entre desde y hacia que pueda ser capturado. Retorna true y escribe su posicion en: *sobre si existe
static bool buscar_captura(const Tablero *t, Pos desde, Pos hacia, Pos *sobre) {
    Pos v[MAX_VECINOS];
    int n = vecinos(desde, v);
    for (int i = 0; i < n; i++) {
        if (contenido(t, v[i]) == PERRO && tablero_es_salto(desde, v[i], hacia)) {
            *sobre = v[i];
            return true;
        }
    }
    return false;
}

int tablero_movimientos_puma(const Tablero *t, Pos destinos[MAX_VECINOS]) {
    Pos v[MAX_VECINOS], w[MAX_VECINOS];
    int n = vecinos(t->puma, v), total = 0;

    for (int i = 0; i < n; i++) {
        if (contenido(t, v[i]) == VACIA && total < MAX_VECINOS) {
            destinos[total++] = v[i];               // movimiento simple
        } else if (contenido(t, v[i]) == PERRO) {
            int m = vecinos(v[i], w);               //posible captura
            for (int j = 0; j < m; j++) {
                if (contenido(t, w[j]) == VACIA &&
                    tablero_es_salto(t->puma, v[i], w[j]) &&
                    total < MAX_VECINOS)
                    destinos[total++] = w[j];
            }
        }
    }
    return total;
}

int tablero_movimientos_perro(const Tablero *t, int id, Pos destinos[MAX_VECINOS]) {
    if (id < 0 || id >= NUM_PERROS || !t->perro_vivo[id]) return 0;
    Pos v[MAX_VECINOS];
    int n = vecinos(t->perros[id], v), total = 0;
    for (int i = 0; i < n; i++)
        if (contenido(t, v[i]) == VACIA)
            destinos[total++] = v[i];
    return total;
}

bool tablero_mover_puma(Tablero *t, Pos destino, int *capturado) {
    *capturado = -1;
    if (!tablero_es_nodo(destino) || contenido(t, destino) != VACIA)
        return false;

    Pos sobre;
    if (tablero_adyacentes(t->puma, destino)) {
        // movimiento simple
    } else if (buscar_captura(t, t->puma, destino, &sobre)) {
        int id = t->casilla[sobre.fila][sobre.col].perro;
        t->casilla[sobre.fila][sobre.col].pieza = VACIA;
        t->casilla[sobre.fila][sobre.col].perro = -1;
        t->perro_vivo[id] = false;
        t->capturados++;
        *capturado = id;
    } else {
        return false;
    }

    t->casilla[t->puma.fila][t->puma.col].pieza = VACIA;
    t->casilla[destino.fila][destino.col].pieza = PUMA;
    t->puma = destino;
    return true;
}

bool tablero_mover_perro(Tablero *t, int id, Pos destino) {
    if (id < 0 || id >= NUM_PERROS || !t->perro_vivo[id]) return false;
    Pos origen = t->perros[id];
    if (!tablero_adyacentes(origen, destino) || contenido(t, destino) != VACIA)
        return false;

    t->casilla[origen.fila][origen.col].pieza = VACIA;
    t->casilla[origen.fila][origen.col].perro = -1;
    t->casilla[destino.fila][destino.col].pieza = PERRO;
    t->casilla[destino.fila][destino.col].perro = id;
    t->perros[id] = destino;
    return true;
}

void tablero_imprimir(const Tablero *t, FILE *salida) {
    for (int f = 0; f < FILAS; f++) {
        fprintf(salida, "  %d ", f);
        for (int c = 0; c < COLUMNAS; c++) {
            const Casilla *cs = &t->casilla[f][c];
            char simbolo = ' ';
            if (cs->existe) {
                if (cs->pieza == PUMA) simbolo = 'P';
                else if (cs->pieza == PERRO) simbolo = 'o';
                else simbolo = '.';
            }
            fprintf(salida, " %c", simbolo);
        }
        fprintf(salida, "\n");
    }
    fprintf(salida, "     0 1 2 3 4\n");
    fprintf(salida, "  Perros capturados: %d\n", t->capturados);
}
