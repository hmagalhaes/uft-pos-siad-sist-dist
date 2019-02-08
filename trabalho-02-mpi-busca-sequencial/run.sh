#!/bin/bash

PROGRAM=busca_sequencial_mpi
SOURCE_EXT=cpp
PROCESSOR_COUNT=4
COMPILER=cpp

clear
rm ${PROGRAM}

echo "Compilando ${PROGRAM} em ${COMPILER}"

case "${COMPILER}" in
    c)
        mpicc -o ${PROGRAM} ${PROGRAM}.c
    ;;
    cpp)
        mpic++ -o ${PROGRAM} ${PROGRAM}.cpp
    ;;
    *)
        echo "Compilador inválido"
        exit 1
    ;;
esac

echo "Iniciando ${PROGRAM} com ${PROCESSOR_COUNT} processors"

mpirun -np ${PROCESSOR_COUNT} --hostfile host_file ${PROGRAM}
