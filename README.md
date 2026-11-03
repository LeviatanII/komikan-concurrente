# Torneo de Komikan Concurrente

Tarea 2 de Sistemas Operativos.

Programa desarrollado en C que simula un torneo de Komikan en tiempo real mediante hilos, mutexes, variables de condición y semáforos POSIX.

## Descripción

El Komikan enfrenta a un Puma contra 12 Perros en un tablero de 5 × 5 intersecciones acoplado a un triángulo. En este torneo no existen turnos: cada ficha se mueve de forma independiente y simultánea.

El programa recibe la cantidad de jugadores que llegan al servidor y la cantidad máxima de mesas que pueden estar activas al mismo tiempo.

En esta etapa el programa valida los argumentos de entrada. El emparejamiento, las mesas y la simulación de las partidas se incorporan en las siguientes etapas.

## Compilación

Desde la carpeta del proyecto, ejecutar:

```
gcc -Wall -Wextra -std=c17 -pthread main.c -o torneo
```

Se utilizan exactamente las opciones de compilación indicadas en la pauta.

## Ejecución

Ejecutar indicando la cantidad de jugadores, el límite de mesas y, opcionalmente, un archivo de configuración:

```
./torneo 4 2
```

En este ejemplo llegan 4 jugadores al servidor y se permiten como máximo 2 mesas activas simultáneamente.

N debe ser un entero mayor o igual a 2 y M un entero mayor o igual a 1. Si N es impar, un jugador queda sin pareja y se informa con un aviso.

## Organización del código

main.c contiene el punto de entrada del programa. Valida los argumentos y muestra los parámetros recibidos.

## Funciones implementadas

uso(): muestra la forma correcta de invocar el programa.

parsear_entero(): convierte un argumento a entero mediante strtol() y rechaza valores vacíos, no numéricos, con caracteres sobrantes, fuera de rango o no positivos.

## Códigos de retorno

0 indica que la ejecución terminó correctamente.

1 indica que hubo un error o una interrupción durante la ejecución.

2 indica que los argumentos son inválidos.
