#!/usr/bin/env bash

#config:
BASE_DIR=/home/tj75qeje
VERSION=10.0.0

# call: get_sub_project name version
get_sub_project () {
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-${2}/${1}-${2}.src.tar.xz
tar -xf ${1}-${2}.src.tar.xz
rm ${1}-${2}.src.tar.xz
mv ${1}-${2}.src ${1}
}

LLVM_SRC=${BASE_DIR}/llvm-${VERSION}.src

cd $BASE_DIR
echo "download llvm version $VERSION into $LLVM_SRC"


#LLVM
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-${VERSION}/llvm-${VERSION}.src.tar.xz
tar -xf llvm-${VERSION}.src.tar.xz
rm llvm-${VERSION}.src.tar.xz

cd $LLVM_SRC/tools

#CLANG
get_sub_project clang $VERSION

# other stuff

get_sub_project lld ${VERSION}

get_sub_project lldb ${VERSION}

get_sub_project polly ${VERSION}

get_sub_project clang-tools-extra ${VERSION}

cd $LLVM_SRC/projects

get_sub_project compiler-rt ${VERSION}

get_sub_project libcxx ${VERSION}

get_sub_project libcxxabi ${VERSION}

get_sub_project libunwind ${VERSION}

##get_sub_project test-suite ${VERSION}

get_sub_project openmp ${VERSION}


