#!/bin/bash

# load required modules
ml git cmake gcc llvm

export PATH=$PATH:/home/tj75qeje/mpich_with_clang_install/bin
export CPATH=$CPATH:/home/tj75qeje/mpich_with_clang_install/include
export LIBRARY_PATH=$LIBRARY_PATH:/home/tj75qeje/mpich_with_clang_install/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/tj75qeje/mpich_with_clang_install/lib

# setup poath to custom mpiu installation (built with clang)
export MPICC=/home/tj75qeje/mpich_with_clang_install/bin/mpicc
export MPICXX=/home/tj75qeje/mpich_with_clang_install/bin/mpicxx

# using mpich:

