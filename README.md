# Torneo de Komikan Concurrente

Tarea 2 de Sistemas Operativos.

Programa desarrollado en C que simula un torneo de Komikan en tiempo real mediante hilos, mutexes, variables de condición y semáforos POSIX.

## Descripción

El Komikan enfrenta a un Puma contra 12 Perros en un tablero de 5 × 5 intersecciones acoplado a un triángulo. En este torneo no existen turnos: cada ficha se mueve de forma independiente y simultánea.

El programa recibe la cantidad de jugadores que llegan al servidor y la cantidad máxima de mesas que pueden estar activas al mismo tiempo.

Actualmente el programa valida los argumentos de entrada, lee el archivo de configuración opcional, construye el tablero con la disposición inicial de las fichas y lo muestra en pantalla. El emparejamiento, las mesas y la simulación concurrente de las partidas se incorporan en las siguientes etapas.

## Compilación

Desde la carpeta del proyecto, ejecutar:

```
gcc -Wall -Wextra -std=c17 -pthread main.c tablero.c config.c -o torneo
```

Se utilizan exactamente las opciones de compilación indicadas en la pauta.

## Ejecución

Ejecutar indicando la cantidad de jugadores, el límite de mesas y, opcionalmente, un archivo de configuración:

```
./torneo 4 2 config.txt
```

En este ejemplo llegan 4 jugadores al servidor, se permiten como máximo 2 mesas activas simultáneamente y se utiliza la configuración de config.txt.

Si no se indica un archivo de configuración, se utilizan los valores por defecto.

N debe ser un entero mayor o igual a 2 y M un entero mayor o igual a 1. Si N es impar, un jugador queda sin pareja y se informa con un aviso.

Por ahora, el programa muestra la configuración utilizada y el tablero con la disposición inicial de las fichas.

## Formato del archivo de configuración

Cada línea contiene una clave y su valor separados por el signo igual:

```
clave = valor
```

Las claves aceptadas son las siguientes.

retardo_min y retardo_max indican, en milisegundos, el retardo mínimo y máximo entre dos movimientos de una misma ficha. Deben ser enteros no negativos y retardo_min no puede ser mayor que retardo_max. Por defecto son 50 y 300.

semilla indica la semilla aleatoria. Permite repetir una ejecución con los mismos valores aleatorios. Debe ser un entero entre 0 e INT_MAX. Si no se indica, se utiliza la hora actual y se muestra al iniciar el programa.

puma indica la posición inicial del puma con el formato fila,col.

perros indica las posiciones iniciales de los 12 perros con el formato fila,col, separadas por espacios.

Las posiciones utilizan las filas y columnas de la matriz descrita en la sección Representación del tablero y deben corresponder a nodos reales.

Todas las claves son opcionales. Las que no aparecen conservan su valor por defecto.

Se permiten líneas vacías y comentarios que comiencen con el carácter numeral.

Se rechazan claves desconocidas o repetidas, líneas sin signo igual, valores no numéricos o con caracteres sobrantes, posiciones que no son nodos, una cantidad de perros distinta de 12, fichas repetidas en la misma posición y líneas de más de 511 caracteres. El mensaje de error indica el archivo y el número de línea.

El archivo config.txt contiene un ejemplo con la disposición por defecto:

```
retardo_min = 50
retardo_max = 300
semilla = 42
puma = 0,2
perros = 5,0 5,1 5,2 5,3 5,4 6,0 6,1 6,2 6,3 6,4 4,0 4,4
```

## Organización del código

main.c contiene el punto de entrada del programa. Valida los argumentos, carga la configuración, inicializa el tablero y lo muestra.

tablero.c y tablero.h definen el tablero, las piezas y las reglas del juego. Implementan las conexiones entre nodos, los movimientos válidos, las capturas del puma y la impresión de la matriz de juego.

config.c y config.h definen los parámetros del torneo. Implementan los valores por defecto y la lectura y validación del archivo de configuración.

config.txt contiene una configuración de ejemplo.

## Funciones implementadas

uso(): muestra la forma correcta de invocar el programa.

parsear_entero(): convierte un argumento a entero mediante strtol() y rechaza valores vacíos, no numéricos, con caracteres sobrantes, fuera de rango o no positivos.

config_defecto(): asigna los retardos por defecto, una semilla basada en la hora actual y la disposición inicial por defecto.

config_cargar(): lee el archivo línea por línea, valida cada clave y su valor, y comprueba que los retardos sean coherentes y que no existan fichas repetidas.

config_mostrar(): muestra los retardos y la semilla utilizados.

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

2 indica que los argumentos o el archivo de configuración son inválidos.
