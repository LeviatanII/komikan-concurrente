# Torneo de Komikan Concurrente

Tarea 2 de Sistemas Operativos.

Programa desarrollado en C que simula un torneo de Komikan en tiempo real mediante hilos, mutexes, variables de condición y semáforos POSIX.

## Descripción

El Komikan enfrenta a un Puma contra 12 Perros en un tablero de 5 × 5 intersecciones acoplado a un triángulo. En este torneo no existen turnos: cada ficha se mueve de forma independiente y simultánea.

El programa recibe la cantidad de jugadores que llegan al servidor y la cantidad máxima de mesas que pueden estar activas al mismo tiempo.

Actualmente el programa valida los argumentos de entrada, construye el tablero con la disposición inicial de las fichas y lo muestra en pantalla. El emparejamiento, las mesas y la simulación concurrente de las partidas se incorporan en las siguientes etapas.

## Compilación

Desde la carpeta del proyecto, ejecutar:

```
gcc -Wall -Wextra -std=c17 -pthread main.c tablero.c -o torneo
```

Se utilizan exactamente las opciones de compilación indicadas en la pauta.

## Ejecución

Ejecutar indicando la cantidad de jugadores, el límite de mesas y, opcionalmente, un archivo de configuración:

```
./torneo 4 2
```

En este ejemplo llegan 4 jugadores al servidor y se permiten como máximo 2 mesas activas simultáneamente.

N debe ser un entero mayor o igual a 2 y M un entero mayor o igual a 1. Si N es impar, un jugador queda sin pareja y se informa con un aviso.

Por ahora, el programa muestra el tablero con la disposición inicial de las fichas.

## Organización del código

main.c contiene el punto de entrada del programa. Valida los argumentos, inicializa el tablero y lo muestra.

tablero.c y tablero.h definen el tablero, las piezas y las reglas del juego. Implementan las conexiones entre nodos, los movimientos válidos, las capturas del puma y la impresión de la matriz de juego.

## Funciones implementadas

uso(): muestra la forma correcta de invocar el programa.

parsear_entero(): convierte un argumento a entero mediante strtol() y rechaza valores vacíos, no numéricos, con caracteres sobrantes, fuera de rango o no positivos.

tablero_disposicion_defecto(): entrega la disposición inicial por defecto del puma y los 12 perros.

tablero_inicializar(): marca los nodos válidos de la matriz y ubica las fichas. Rechaza posiciones que no son nodos o que están repetidas.

tablero_es_nodo(): indica si una posición de la matriz corresponde a un nodo real del tablero.

tablero_adyacentes(): indica si dos nodos están unidos por una línea del tablero.

tablero_es_salto(): indica si tres nodos consecutivos forman una línea recta, condición necesaria para una captura.

tablero_movimientos_puma(): calcula los destinos válidos del puma, incluyendo movimientos simples y capturas.

tablero_movimientos_perro(): calcula los destinos válidos de un perro.

tablero_mover_puma(): aplica un movimiento del puma si es válido. Si corresponde a una captura, retira al perro saltado e incrementa el contador de capturas.

tablero_mover_perro(): aplica un movimiento de un perro si el destino es adyacente y está libre.

tablero_imprimir(): muestra la matriz de juego y la cantidad de perros capturados.

## Representación del tablero

El tablero se representa como una matriz de 7 filas por 5 columnas. Las filas 2 a 6 forman la cuadrícula de 5 × 5, la fila 1 contiene la base del triángulo en las columnas 1 y 3, y la fila 0 contiene el vértice del triángulo en la columna 2. Las 7 casillas restantes de la matriz no son nodos y nunca pueden ocuparse.

```
  0      T
  1    .   .
  2  . . . . .
  3  . . . . .
  4  . . . . .
  5  . . . . .
  6  . . . . .
     0 1 2 3 4
```

En total existen 28 nodos y 48 conexiones. En la cuadrícula los nodos se conectan solamente en horizontal y vertical. Las conexiones del triángulo se almacenan en una tabla: el vértice se une con ambos nodos de la base y con el centro de la fila 2, los nodos de la base se unen entre sí, y cada uno se une con la esquina y la columna interior de su lado.

Cada casilla guarda si es un nodo, la pieza que contiene y, en el caso de un perro, su identificador. El tablero también guarda la posición del puma, la posición de cada perro, cuáles siguen en juego y la cantidad de capturas.

## Reglas de movimiento

El puma y los perros se mueven a un nodo adyacente libre.

El puma captura a un perro saltando sobre él hacia un nodo libre ubicado en la misma línea recta. Se considera que tres nodos están en línea recta cuando ambos tramos están conectados y avanzan en la misma dirección. Esta regla incluye los saltos del triángulo, por ejemplo desde el vértice sobre el centro de la fila 2 hacia la fila 3.

Los perros no pueden capturar.

El puma gana al capturar 6 perros. Los perros ganan cuando el puma no tiene movimientos válidos.

La disposición inicial por defecto ubica al puma en el vértice del triángulo y a los perros en las filas 5 y 6 junto con las esquinas de la fila 4. En la matriz impresa, P representa al puma, o a un perro y el punto a un nodo vacío.

## Códigos de retorno

0 indica que la ejecución terminó correctamente.

1 indica que hubo un error o una interrupción durante la ejecución.

2 indica que los argumentos son inválidos.
