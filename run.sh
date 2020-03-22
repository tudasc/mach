#!/bin/bash

#using openmpi:
#OMPI_CXX=clang++ mpicxx -fopenmp -Xclang -load -Xclang build/experimentpass/libexperimentpass.so  $1

# using mpich:
$MPICXX -cxx=clang++ -fopenmp -Xclang -load -Xclang build/msg_order_relax_checker/libmsg_order_relax_checker.so  $1
