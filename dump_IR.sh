#!/bin/bash

#using openmpi:
#OMPI_CXX=clang++ mpicxx -fopenmp -Xclang -load -Xclang build/experimentpass/libexperimentpass.so  $1

MPICXX=/home/tj75qeje/mpich_with_clang_install/bin/mpicxx

# using mpich:
$MPICXX -cxx=clang++ -fopenmp -Xclang -load -Xclang build/experimentpass/libexperimentpass.so  $1
