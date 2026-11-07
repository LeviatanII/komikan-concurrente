#!/bin/bash

echo " Compilando torneo"

gcc -Wall -Wextra -std=c17 -pthread \
    main.c tablero.c config.c partida.c juez.c servidor.c \
    -o torneo


if [ $? -eq 0 ]; then
	echo ""
	echo " Compilacion exitosa"
else
	echo ""
	echo " Error de compilacion"
	exit 1
fi
