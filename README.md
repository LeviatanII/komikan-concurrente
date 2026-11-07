# Torneo de Komikan Concurrente

Tarea 2 de Sistemas Operativos.

Programa desarrollado en C que simula un torneo de Komikan en tiempo real mediante hilos, mutexes, variables de condición y semáforos POSIX.

## Descripción

El Komikan enfrenta a un Puma contra 12 Perros en un tablero de 5 × 5 intersecciones acoplado a un triángulo. En este torneo no existen turnos: cada ficha se mueve de forma independiente y simultánea.

El programa recibe la cantidad de jugadores que llegan al servidor y la cantidad máxima de mesas que pueden estar activas al mismo tiempo.

Cada partida utiliza 13 hilos: uno para el puma y uno para cada perro. Cada hilo espera un retardo aleatorio, elige un movimiento válido y lo aplica sobre un tablero compartido. Si dos fichas intentan ocupar la misma casilla al mismo tiempo, los mutex del tablero garantizan que solo una lo consiga.

Cada mesa tiene un hilo juez independiente. El juez inicializa el tablero y sus primitivas de sincronización, crea los hilos de las fichas, revisa el tablero tras cada movimiento, registra la partida en un archivo de log, detecta la victoria y libera los recursos al terminar.

Un servidor central recibe a los N jugadores, cada uno representado por un hilo que llega en un momento aleatorio. A medida que llegan, el servidor los empareja de a dos: un jugador controla al puma y el otro a los perros. Cada pareja recibe una mesa libre, donde un juez dirige su partida. Nunca hay más de M mesas activas: las parejas que no encuentran mesa esperan de forma pasiva en un semáforo.

Si un jugador falla durante una partida, el juez lo detecta, aborta la partida y declara ganador al oponente por abandono. Al presionar Ctrl+C, el servidor detiene la llegada de jugadores, interrumpe de forma ordenada las partidas activas y libera todos los recursos antes de terminar.

## Compilación

Desde la carpeta del proyecto, ejecutar:

```
gcc -Wall -Wextra -std=c17 -pthread main.c tablero.c config.c partida.c juez.c servidor.c -o torneo
```

Se utilizan exactamente las opciones de compilación indicadas en la pauta.

## Ejecución

Ejecutar indicando la cantidad de jugadores, el límite de mesas y, opcionalmente, un archivo de configuración:

```
./torneo 4 2 config.txt
```

En este ejemplo llegan 4 jugadores al servidor, se permiten como máximo 2 mesas activas simultáneamente y se utiliza la configuración de config.txt.

Si no se indica un archivo de configuración, se utilizan los valores por defecto.

N debe ser un entero entre 2 y 1000 y M un entero entre 1 y 100. Estos límites evitan agotar los hilos del sistema, ya que cada jugador es un hilo y cada mesa activa utiliza 14 hilos. Si N es impar, un jugador queda sin pareja y se informa con un aviso.

En la consola se muestran la llegada de los jugadores, las parejas formadas, la asignación y liberación de mesas, las capturas y el resultado de cada partida. Al final se muestra un resumen del torneo. El detalle de cada movimiento y el tablero actualizado de cada partida se escriben en el archivo partida_N.log.

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

fok_israel indica la probabilidad, entre 0 y 100, de que un jugador falle durante una partida. Permite probar el manejo de fallos. Por defecto es 0, es decir, ningún jugador falla.

Todas las claves son opcionales. Las que no aparecen conservan su valor por defecto.

Se permiten líneas vacías y comentarios que comiencen con el carácter numeral.

Se rechazan claves desconocidas o repetidas, líneas sin signo igual, valores no numéricos o con caracteres sobrantes, posiciones que no son nodos, una cantidad de perros distinta de 12, fichas repetidas en la misma posición y líneas de más de 511 caracteres. El mensaje de error indica el archivo y el número de línea.

El archivo config.txt contiene un ejemplo con la disposición por defecto y sin fallos:

```
retardo_min = 50
retardo_max = 300
semilla = 42
puma = 0,2
perros = 5,0 5,1 5,2 5,3 5,4 6,0 6,1 6,2 6,3 6,4 4,0 4,4
fok_israel = 0
```

## Organización del código

main.c contiene el punto de entrada del programa. Valida los argumentos, carga la configuración, inicia el servidor y muestra el resumen del torneo.

tablero.c y tablero.h definen el tablero, las piezas y las reglas del juego. Implementan las conexiones entre nodos, los movimientos válidos, las capturas del puma y la impresión de la matriz de juego.

config.c y config.h definen los parámetros del torneo. Implementan los valores por defecto y la lectura y validación del archivo de configuración.

partida.c y partida.h implementan la partida concurrente. Definen el estado compartido de una mesa, los hilos del puma y los perros, las zonas de bloqueo y las esperas entre movimientos.

servidor.c y servidor.h implementan el servidor de emparejamiento: la llegada de los jugadores, la sala de espera, la formación de parejas, el semáforo de mesas libres y la asignación y liberación de mesas.

juez.c y juez.h implementan el hilo juez de cada mesa: inicialización, revisión del tablero, log, detección de victoria y liberación de recursos.

config.txt contiene una configuración de ejemplo.

## Funciones implementadas

uso(): muestra la forma correcta de invocar el programa.

parsear_entero(): convierte un argumento a entero mediante strtol() y rechaza valores vacíos, no numéricos, con caracteres sobrantes, fuera de rango o no positivos.

config_defecto(): asigna los retardos por defecto, una semilla basada en la hora actual y la disposición inicial por defecto.

config_cargar(): lee el archivo línea por línea, valida cada clave y su valor, y comprueba que los retardos sean coherentes y que no existan fichas repetidas.

config_mostrar(): muestra la probabilidad de fallo, los retardos y la semilla utilizados.

servidor_iniciar(): reserva la memoria de los jugadores y las mesas, e inicializa el semáforo de mesas libres con el valor M, el mutex y la variable de condición del servidor. Si algún paso falla, libera lo que alcanzó a reservar.

servidor_ejecutar(): crea los N hilos de los jugadores y espera que terminen. Si no puede crear un hilo, ajusta la cantidad de jugadores para que nadie espere a uno que nunca llegará.

servidor_mostrar_resumen(): muestra la cantidad de partidas, sus resultados, los jugadores sin pareja y el máximo de mesas activas al mismo tiempo.

servidor_destruir(): destruye el semáforo, el mutex y la variable de condición, y libera la memoria del servidor.

hilo_jugador(): ciclo de un jugador. Llega al servidor y, si hay alguien esperando, forma pareja y se encarga de pedir la mesa. Si no, espera en la sala hasta que llegue otro jugador. Si ya llegaron todos, queda sin pareja.

esperar_llegada(): simula el tiempo de llegada del jugador mediante pthread_cond_timedwait(). Retorna antes si el servidor se cierra con Ctrl+C.

terminar_pareja(): marca a ambos jugadores de una pareja como terminados y despierta al que espera.

servidor_interrumpir(): marca el cierre del servidor, despierta a los jugadores que llegan o esperan pareja y notifica a cada partida activa que debe terminar.

hilo_senales(): hilo dedicado a Ctrl+C. Espera SIGINT con sigwait() y llama a servidor_interrumpir().

jugar_partida(): espera una mesa libre con sem_wait(), la marca como ocupada, crea el hilo juez de la mesa y espera su término con pthread_join(). Luego libera la mesa, registra el resultado y hace sem_post(). Si el servidor se cerró mientras esperaba, la pareja se retira y devuelve el turno del semáforo.

hilo_juez(): hilo juez de una mesa. Inicializa la partida, la registra en su mesa, crea las fichas, espera eventos con pthread_cond_wait(), detecta fallas de jugadores, revisa el tablero tras cada evento, espera a las fichas con pthread_join(), comprueba que no queden mutex tomados y libera los recursos.

buscar_falla(): busca una ficha cuyo hilo terminó mientras la partida seguía activa y sin haber sido capturada.

declarar_abandono(): aborta la partida y declara ganador al oponente del jugador que falló.

revisar_tablero(): bloquea el tablero completo, registra la matriz de juego en el log, verifica su consistencia y revisa las condiciones de victoria.

estado_consistente(): comprueba que exista un solo puma, que cada perro en juego esté en la casilla registrada y que la cantidad de perros coincida con las capturas.

juez_texto_resultado(): entrega el texto que describe un resultado, indicando si fue por abandono o si la partida se interrumpió.

partida_inicializar(): inicializa el tablero, abre el archivo de log y crea los mutex y variables de condición de la partida.

partida_destruir(): cierra el log y destruye los mutex y variables de condición.

partida_crear_fichas(): crea los 13 hilos y asigna a cada ficha una semilla propia derivada de la semilla de la configuración.

partida_esperar_fichas(): espera mediante pthread_join() que terminen los hilos creados.

partida_bloquear_todo() y partida_desbloquear_todo(): bloquean y liberan todas las casillas y el mutex del estado respetando el orden global.

partida_anunciar_fin(): marca la partida como terminada, guarda el resultado y despierta a las fichas y al juez.

partida_interrumpir(): termina la partida por cierre del servidor.

partida_mutex_retenidos(): cuenta los mutex de la partida que siguen tomados mediante pthread_mutex_trylock().

limpiar_zona() y limpiar_estado(): manejadores de limpieza que liberan las casillas y el mutex del estado si el hilo termina mientras los tiene tomados.

marcar_fin_ficha(): manejador de limpieza que marca la ficha como terminada y avisa al juez, tanto si el hilo termina normalmente como si falla.

simular_falla(): simula una falla interna terminando el hilo con pthread_exit() mientras tiene sus casillas y el mutex del estado tomados.

hilo_perro(): ciclo de un perro. Espera un retardo, bloquea su zona del tablero, elige al azar un destino libre y se mueve. Termina cuando la partida finaliza o cuando el perro es capturado.

hilo_puma(): ciclo del puma. Espera un retardo, bloquea su zona del tablero y se mueve, prefiriendo una captura si existe. Si no tiene movimientos válidos, avisa al juez.

calcular_zona(): obtiene las casillas a distancia 1 (perro) o 2 (puma) de una posición, ordenadas por índice.

bloquear_zona() y desbloquear_zona(): toman los mutex de una zona en orden ascendente y los liberan en orden inverso.

partida_retardo_aleatorio(): calcula un retardo aleatorio entre retardo_min y retardo_max mediante rand_r().

partida_tiempo_limite(): calcula el instante absoluto que se usa como límite en pthread_cond_timedwait().

esperar_retardo(): espera un tiempo aleatorio entre retardo_min y retardo_max mediante pthread_cond_timedwait(). Retorna antes si la partida termina.

partida_activa(): indica si las fichas pueden seguir moviéndose, es decir, si la partida no terminó y el puma no alcanzó las 6 capturas.

avisar_juez(): cuenta un evento y despierta al juez mediante pthread_cond_signal().

registrar_movimiento(): escribe un movimiento en el log y avisa al juez mientras la ficha mantiene bloqueadas sus casillas.

registrar_rechazo(): escribe en el log un movimiento que no cumple las reglas y no se aplica.

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

## Sincronización del tablero

Cada casilla del tablero tiene su propio pthread_mutex_t. Antes de decidir un movimiento, una ficha bloquea su zona: las casillas que necesita leer y modificar. La zona de un perro contiene su casilla y las adyacentes. La zona del puma alcanza distancia 2, porque una captura involucra la casilla del perro saltado y la casilla de destino.

Mientras mantiene su zona bloqueada, la ficha calcula los movimientos válidos, elige uno y lo aplica. Como la decisión y el movimiento ocurren dentro de la misma sección crítica, ninguna otra ficha puede ocupar el destino entre ambos pasos.

Si dos fichas compiten por la misma casilla libre, esa casilla pertenece a las zonas de ambas. La primera que obtiene su mutex realiza el movimiento. La segunda, al obtenerlo, ve la casilla ocupada y elige otro destino.

Cuando el puma captura a un perro, la casilla de ese perro forma parte de la zona del puma. Por eso el hilo del perro no puede moverse durante la captura. Al bloquear su siguiente zona, el perro comprueba que sigue en juego y, si fue capturado, su hilo termina.

La posición de cada ficha solo la modifica su propio hilo. Por eso cada hilo puede leer su posición antes de bloquear la zona.

El estado general de la partida (si terminó, el resultado, los contadores y el archivo de log) se protege con un mutex adicional.

## Servidor y emparejamiento

Cada uno de los N jugadores es un hilo. Después de un retardo aleatorio, el jugador llega al servidor y entra a una sala de espera protegida por el mutex del servidor.

Si en la sala hay un jugador esperando, se forma una pareja: el que esperaba controla al puma y el que llega controla a los perros. Si la sala está vacía, el jugador queda esperando en pthread_cond_wait() hasta que otro jugador forme pareja con él.

El jugador que completa la pareja se encarga de pedir una mesa. El otro espera en la variable de condición del servidor hasta que su partida termine.

Si N es impar, el último jugador en quedar solo detecta que ya llegaron todos los jugadores y se retira sin pareja.

## Límite de mesas y espera pasiva

El servidor utiliza un semáforo POSIX (sem_t) inicializado con el valor M, que representa las mesas libres.

Antes de ocupar una mesa, la pareja ejecuta sem_wait(). Si hay una mesa libre, el semáforo se decrementa y la pareja continúa. Si las M mesas están ocupadas, el hilo queda bloqueado dentro de sem_wait() sin consumir CPU, hasta que otra partida termine.

Al terminar una partida, primero se marca la mesa como libre y luego se ejecuta sem_post(). Así, la pareja que despierta siempre encuentra una mesa disponible en el arreglo de mesas.

Como el semáforo solo permite M decrementos sin un sem_post() intermedio, nunca existen más de M mesas activas. El servidor registra el máximo de mesas activas observado y lo muestra en el resumen final.

## Juez de mesa

Cada mesa tiene un hilo juez. Las fichas no se comunican con el juez directamente: después de cada movimiento incrementan un contador de eventos y lo despiertan con pthread_cond_signal(). El juez duerme en pthread_cond_wait() mientras no existan eventos nuevos.

Al despertar, el juez bloquea todas las casillas y el mutex del estado. Así obtiene una instantánea consistente del tablero completo, en la que ninguna ficha está a mitad de un movimiento. Sobre esa instantánea registra la matriz de juego y revisa las condiciones de victoria.

Todo movimiento se valida dentro de la sección crítica de la ficha mediante las reglas del tablero. Si un movimiento no cumple las reglas de adyacencia o captura, se rechaza y se registra en el log. Además, el juez verifica en cada revisión que el tablero sea consistente. Si detecta una inconsistencia, aborta la partida sin ganador.

El puma gana al capturar 6 perros. Desde ese momento las fichas dejan de moverse, aunque el juez todavía no haya anunciado el resultado.

Los perros ganan cuando, en una revisión del juez, el puma no tiene movimientos válidos. Si el puma intenta moverse y no puede, también avisa al juez para que revise el tablero.

Al detectar la victoria, el juez marca la partida como terminada y despierta a todas las fichas con pthread_cond_broadcast(). Luego espera a los 13 hilos con pthread_join(), cierra el log y destruye los mutex y variables de condición, dejando la mesa libre.

## Manejo de fallos de jugadores

El jugador del puma controla el hilo del puma y el jugador de los perros controla los 12 hilos de los perros. Una falla de cualquiera de esos hilos se considera una falla de su jugador.

Para probar este caso, fok_israel indica la probabilidad de que en una partida falle un jugador. Al crear las fichas, se elige al azar si alguna fallará, cuál y en qué ciclo. Esa ficha termina abruptamente con pthread_exit() en el peor momento posible: con sus casillas y el mutex del estado tomados.

Un mutex POSIX solo puede ser liberado por el hilo que lo tomó, por lo que otro hilo no puede desbloquearlo. Para que una falla no deje primitivas tomadas, cada ficha registra manejadores de limpieza con pthread_cleanup_push() justo después de tomar sus casillas y el mutex del estado. Si el hilo termina con pthread_exit(), los manejadores se ejecutan en orden inverso y liberan ambos.

Además, cada ficha registra al iniciar un manejador que la marca como terminada y despierta al juez. El juez revisa las fichas terminadas: un perro capturado o el fin de la partida son terminaciones normales. Cualquier otra terminación es una falla. En ese caso, el juez aborta la partida, declara ganador al oponente por abandono y despierta a las demás fichas.

Antes de destruir las primitivas, el juez espera a los 13 hilos, incluido el que falló, y comprueba con pthread_mutex_trylock() que ningún mutex quedó tomado. El resultado de esa comprobación se registra en el log.

## Manejo de Ctrl+C

SIGINT se bloquea con pthread_sigmask() antes de crear cualquier hilo, por lo que todos la heredan bloqueada. Un hilo dedicado la recibe mediante sigwait(). Así, la señal se atiende como un evento normal del programa, fuera de un manejador asíncrono, y puede usar mutex y variables de condición sin riesgo.

Al recibir Ctrl+C, el servidor marca que está cerrando y despierta a los jugadores con pthread_cond_broadcast(). Los jugadores que aún no llegaban dejan de llegar y los que esperaban pareja se retiran.

Cada juez registra su partida en el servidor mientras está activa. El servidor recorre las mesas y notifica a cada partida, que termina de forma ordenada: las fichas despiertan de inmediato, el juez espera sus hilos y libera la mesa.

Las parejas que esperaban mesa en sem_wait() despiertan cuando una partida libera su mesa. Al ver que el servidor está cerrando, se retiran y hacen sem_post(), de modo que la siguiente pareja en espera también despierta y se retira.

Cuando todos los jugadores terminaron, el hilo de señales se cancela, ya que sigwait() es un punto de cancelación. Mientras atiende una señal, desactiva su cancelación para no terminar con el mutex del servidor tomado.

## Administración de recursos

Cada juez cierra el log de su partida y destruye los mutex y variables de condición de la mesa después de esperar a todas las fichas.

Al terminar el torneo, normalmente o por Ctrl+C, el servidor espera a todos los jugadores y al hilo de señales, destruye el semáforo, el mutex y la variable de condición, y libera la memoria reservada.

El programa no utiliza memoria compartida ni semáforos con nombre, por lo que no deja recursos huérfanos en el sistema operativo al terminar.

Se verificó con valgrind que no quedan fugas de memoria ni descriptores de archivo abiertos, tanto en ejecuciones completas como interrumpidas con Ctrl+C y con fallas de jugadores.

## Prevención de deadlock

Dentro de una partida, todos los hilos, incluido el juez, bloquean las casillas en orden ascendente de índice (fila × 5 + columna) y toman el mutex del estado al final. Como todos siguen el mismo orden global, no puede formarse una espera circular y por lo tanto no existe deadlock.

Los mutex se liberan en orden inverso al que se tomaron, también cuando los libera un manejador de limpieza.

El mutex del servidor nunca se mantiene tomado mientras se espera el semáforo de mesas ni mientras se juega una partida. La única situación en que se anidan el mutex del servidor y el de una partida es al notificar Ctrl+C, siempre en ese orden: primero el del servidor y luego el del estado de la partida. Ningún hilo los toma en el orden contrario.

## Espera sin busy-waiting

Las parejas que no tienen mesa esperan bloqueadas en sem_wait(). Los jugadores que esperan pareja o el término de su partida esperan en pthread_cond_wait().

El juez espera eventos con pthread_cond_wait() y no revisa el tablero mientras no ocurra un movimiento.

Los retardos entre movimientos se realizan con pthread_cond_timedwait() sobre la variable de condición de fin de partida. Mientras espera, el hilo no consume CPU y libera el mutex del estado.

Cuando la partida termina, pthread_cond_broadcast() despierta inmediatamente a todas las fichas, sin esperar a que se cumplan sus retardos. Las esperas se repiten en un ciclo para tolerar despertares espurios.

## Mensajes de salida

Al iniciar se muestran N, M, el archivo de configuración utilizado, la probabilidad de fallo, los retardos y la semilla. Si N es impar, se muestra un aviso.

En la consola se muestran los eventos del servidor:

Llega jugador: indica que un jugador llegó al servidor.

Pareja formada: indica qué jugador controla al puma y cuál a los perros.

Esperando mesa libre: indica que una pareja quedó bloqueada en el semáforo porque las M mesas están ocupadas.

Mesa M: partida P: indica la mesa y el número de partida asignados a una pareja.

Mesa liberada: indica que la partida de una mesa terminó y la mesa quedó disponible.

Queda sin pareja: indica que un jugador no pudo formar pareja.

Ctrl+C recibido: indica que el servidor comenzó a cerrarse.

Retirada por cierre: indica que una pareja que esperaba mesa no jugará.

Al final se muestra un resumen con las partidas jugadas, las victorias del puma y de los perros, las partidas interrumpidas, las partidas sin ganador, los abandonos, los jugadores sin pareja y el máximo de mesas activas al mismo tiempo. Si el torneo se interrumpió, también se indica cuántos jugadores no llegaron.

También se muestran los eventos principales de cada mesa:

Comienza la partida: indica el inicio de una partida, la mesa en que se juega y el nombre de su archivo de log.

El puma captura al perro: indica una captura y la cantidad acumulada.

Abandona la partida: indica que el juez detectó la falla de un jugador.

Fin de la partida: indica la cantidad total de movimientos y el resultado, que puede ser una victoria normal, una victoria por abandono o una partida interrumpida.

Cada partida genera además el archivo partida_N.log, donde N es el número de la partida. Contiene los movimientos numerados en el orden en que se aplicaron, los movimientos rechazados, la matriz de juego registrada por el juez en cada revisión, las fallas e interrupciones, y la cantidad de mutex retenidos al cerrar la mesa.

Los movimientos se registran mientras la ficha mantiene bloqueadas sus casillas. Así, el orden del log coincide con el orden real en que se aplicaron sobre el tablero.

## Códigos de retorno

0 indica que la ejecución terminó correctamente.

1 indica que el torneo se interrumpió con Ctrl+C o que hubo un error durante la ejecución, por ejemplo si no se pudieron crear los hilos o alguna partida terminó sin ganador.

2 indica que los argumentos o el archivo de configuración son inválidos.

## Pruebas realizadas

Compilación: se compiló con el comando de la sección Compilación, utilizando las opciones de la pauta, sin advertencias.

Ejecución normal: se ejecutó ./torneo 7 2 config.txt. Se formaron 3 parejas, un jugador quedó sin pareja por ser N impar y la tercera pareja esperó una mesa libre. Nunca hubo más de 2 mesas activas.

Validación de argumentos: se ejecutó el programa sin argumentos, con un solo argumento, con N o M iguales a 0, no numéricos, con caracteres sobrantes o fuera de rango, y con un archivo de configuración inexistente.

Validación de la configuración: se probaron archivos con una clave desconocida, fok_israel fuera de rango, retardo_min mayor que retardo_max y una posición que no es nodo del tablero.

Pruebas de carga: se ejecutó ./torneo 200 8 con retardos de 0 a 3 ms y fok_israel = 30, para generar muchas competencias simultáneas por las casillas.

Revisión de los logs: se comprobó en los archivos partida_N.log que los movimientos estén numerados en orden, que cada partida termine con un resultado y que se registren 0 mutex retenidos al cerrar la mesa.

Fallo de jugadores: se utilizó fok_israel = 100 para forzar la falla de un jugador en cada partida, comprobando que el juez detecte el abandono y declare ganador al oponente.

Cancelación por señal: se presionó Ctrl+C a mitad del torneo, con partidas en curso y parejas esperando mesa.

Limpieza de recursos: se ejecutó valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes en ejecuciones completas, con fallas de jugadores e interrumpidas con Ctrl+C.

## Resultados

Las pruebas realizadas presentaron el comportamiento esperado. El programa compiló sin advertencias y rechazó los argumentos y archivos de configuración inválidos con el código de retorno 2.

El servidor formó las parejas correctamente, informó al jugador sin pareja y nunca superó el límite de M mesas activas. En la prueba de carga se jugaron 100 partidas con el máximo de 8 de 8 mesas activas.

Los logs registraron los movimientos en orden, cada partida terminó con un resultado y ninguna partida cerró su mesa con mutex retenidos.

Ante la falla de un jugador, el juez abortó la partida y declaró ganador al oponente por abandono. Al presionar Ctrl+C, las partidas en curso se interrumpieron de forma ordenada, las parejas en espera se retiraron y el programa terminó con el código de retorno 1.

valgrind no reportó errores, fugas de memoria ni descriptores de archivo abiertos, aparte de la entrada, salida y salida de error estándar.
