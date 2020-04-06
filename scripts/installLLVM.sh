#!/usr/bin/env bash


# specify the install path of libedit
export CPATH=$LIBRARY_PATH:/home/tj75qeje/.apps/libedit/include
export LIBRARY_PATH=$LIBRARY_PATH:/home/tj75qeje/.apps/libedit/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/tj75qeje/.apps/libedit/lib

export CMAKE_PREFIX_PATH=/home/tj75qeje/.apps/libedit

#Set env-Variables
: ${CC:="gcc"}
: ${CXX:="g++"}
: ${CFLAGS:=""}
: ${CXXFLAGS:="-std=c++17"}
: ${LDFLAGS:=""}
: ${INCFLAGS:=""}
: ${RECOMPILE:=true}
: ${SKIP_TESTS:=true}
: ${CPUS:=24}
##: ${CPUS:=$(nproc)}

#location of llvm src
SRC=/home/tj75qeje/llvm-10.0.0.src
# where to install llvm
INSTALLDIR=/work/scratch/tj75qeje/llvm_install_tmp/

LLVM_BUILD=/work/scratch/tj75qeje/llvm_build/

SOURCEDIR_LLVM=${SRC}


if [[ -d $LLVM_BUILD ]]; then
        read -p "$LLVM_BUILD already exists. Shall it be deleted? (y/n) " yn
        case $yn in
            [Yy] ) rm -rf $LLVM_BUILD; mkdir -p $LLVM_BUILD ;;
            [Nn] ) echo -e "\e[32mContinue installation\e[0m" ;;
        esac
else
    mkdir -p $LLVM_BUILD
fi


cd $LLVM_BUILD

#build llvm
echo -e "\e[32mBuild LLVM\e[0m" | tee --append $INSTALL_LOG

    #build llvm
cmake $SOURCEDIR_LLVM -G "Unix Makefiles" \
-DCMAKE_CXX_STANDARD=17 -DLLVM_ENABLE_WARNINGS=OFF -DLLVM_ENABLE_WERROR=OFF -DCMAKE_BUILD_TYPE=RELEASE -DLLVM_ENABLE_ASSERTIONS=ON \
        -DCMAKE_INSTALL_PREFIX=$INSTALLDIR 

make -j $CPUS
make install



