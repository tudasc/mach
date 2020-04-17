#!/bin/bash
if [[ "$1" == "--rebuild" || "$1" == "re" ]]; then
    rm -rf build 
fi

mkdir -p build
cd build
#CC=clang CXX=clang++
cmake -DCMAKE_BUILD_TYPE=Debug ..
#cmake -DCMAKE_BUILD_TYPE=Release ..
make -j 8
