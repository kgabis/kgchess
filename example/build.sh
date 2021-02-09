#!/bin/bash

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    gcc sdl_game.c ../kgchess.c -o sdl_game `sdl2-config --cflags --libs` -lSDL2_image
elif [[ "$OSTYPE" == "darwin"* ]]; then
    gcc sdl_game.c ../kgchess.c -o sdl_game -F/Library/Frameworks -framework SDL2 -framework SDL2_image
else
    echo "System not supported"
    exit 1
fi