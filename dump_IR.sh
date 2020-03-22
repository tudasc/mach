#!/bin/bash

#using openmpi:
#OMPI_CXX=clang++ mpicxx -fopenmp -Xclang -load -Xclang build/experimentpass/libexperimentpass.so  $1

# using mpich:
$MPICXX -cxx=clang++ -fopenmp -Xclang -load -Xclang build/dump_ir_pass/libdump_ir_pass.so  $1
