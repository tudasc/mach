#!/bin/bash

#using openmpi:
#OMPI_CXX=clang++ mpicxx -fopenmp -Xclang -load -Xclang build/experimentpass/libexperimentpass.so  $1

# using mpich:
if [ ${1: -2} == ".c" ]; then
$MPICC -cc=clang -fopenmp -Xclang -load -Xclang build/msg_order_relax_checker/libmsg_order_relax_checker.so  $1
elif [ ${1: -4} == ".cpp" ]; then
$MPICXX -cxx=clang++ -fopenmp -Xclang -load -Xclang build/msg_order_relax_checker/libmsg_order_relax_checker.so  $1
else
echo "Unknown file suffix, use this script with .c or .cpp files"
fi
