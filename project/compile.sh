#!/bin/bash

echo "compilazione"

#Pulisce la vecchia cartella di build (opzionale
if [ -d "build" ]; then
    rm -rf build
fi

mkdir build
cd build

# Genera il Makefile tramite CMake
# Se fallisce, lo script si ferma (&&)
cmake .. && \

# Compila il progetto
# Se cmake ha avuto successo, lancia make
make

# 6. Verifica finale
if [ $? -eq 0 ]; then
    echo "--- Compilazione completata con successo! ---"
    echo "Trovi gli eseguibili nella cartella 'build'"
else
    echo "ERRORE DURANTE LA COMPILAZIONE"
fi