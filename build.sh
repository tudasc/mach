#!/bin/bash
if [[ "$1" == "--rebuild" || "$1" == "re" ]]; then
    rm -rf build 
fi

mkdir -p build
cd build
#CC=clang CXX=clang++ cmake ..
cmake ..
make -j 8
