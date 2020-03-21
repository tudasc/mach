#!/usr/bin/env bash

if [[ "$DEBUG" == true ]] ; then
    echo "\e[31mVerbose output\e[0m"
    set -x
else
    set +x
fi
set -e
set -u

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
: ${SKIP_TESTS:=false}
: ${CPUS:=24}
##: ${CPUS:=$(nproc)}

#location of llvm src
SRC=/home/tj75qeje/llvm-9.0.1.src
# where to install llvm
INSTALLDIR_FINAL=/work/scratch/tj75qeje/llvm-9.0.1.install
# name of modulefile
MODULEFILE=/home/tj75qeje/.mouldefiles/llvm/9.0.1.lua

LLVM_BUILD=/work/scratch/tj75qeje/llvm_build/
INSTALLDIR_TMP=/work/scratch/tj75qeje/llvm_install_tmp/

SOURCEDIR_LLVM=${SRC}
INSTALL_LOG=${SRC}/llvm_logs/log_llvm_install_$(date +%s).log

echo -e "\e[92mSet env-variables: "
echo -e "Build LLVM with $CPUS core(s)"
echo -e "RECOMPILE (with clang): $RECOMPILE"
echo -e "Skip tests: $SKIP_TESTS"
echo -e "CC=$CC"
echo -e "CXX=$CXX"
echo -e "CFLAGS=$CFLAGS"
echo -e "CXXFLAGS=$CXXFLAGS"
echo -e "LDFLAGS=$LDFLAGS"
echo -e "INCFLAGS=$INCFLAGS\e[0m"

sleep 3s


if [[ "$RECOMPILE" == "true" ]]; then
    INSTALLDIR=$INSTALLDIR_TMP
else
    INSTALLDIR=$INSTALLDIR_FINAL
fi

if [[ -d $LLVM_BUILD ]]; then
        read -p "$LLVM_BUILD already exists. Shall it be deleted? (y/n) " yn
        case $yn in
            [Yy] ) rm -rf $LLVM_BUILD; mkdir -p $LLVM_BUILD ;;
            [Nn] ) echo -e "\e[32mContinue installation\e[0m" ;;
        esac
else
    mkdir -p $LLVM_BUILD
fi
mkdir -p ${SRC}/llvm_logs
touch $INSTALL_LOG

cd $LLVM_BUILD

#build llvm
echo -e "\e[32mBuild LLVM\e[0m" | tee --append $INSTALL_LOG


BUILT_WITH_CLANG=false
while true; do
    #build llvm
cmake $SOURCEDIR_LLVM -G "Unix Makefiles" \
        -DCMAKE_CXX_STANDARD=17 \
        -DLLVM_TARGETS_TO_BUILD=X86 \
        -DC_INCLUDE_DIRS=$INCFLAGS\
        -DLLVM_ENABLE_WERROR=OFF \
        -DCMAKE_BUILD_TYPE=DEBUG \
        -DCMAKE_C_COMPILER=$CC \
        -DCMAKE_CXX_COMPILER=$CXX \
        -DCMAKE_EXE_LINKER_FLAGS=$LDFLAGS \
        -DCMAKE_CXX_FLAGS_RELEASE=$CXXFLAGS \
        -DCMAKE_C_FLAGS_RELEASE=$CFLAGS \
        -DCMAKE_INSTALL_PREFIX=$INSTALLDIR \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON |tee --append $INSTALL_LOG


    echo -e "\n\n\\e[32mnBuild with $CPUS CPU cores\e[0m\n" | tee --append $INSTALL_LOG
    make -j ${CPUS} 2>&1 | tee --append $INSTALL_LOG; test ${PIPESTATUS[0]} -eq 0

    echo -e "\n\n\n\e[32mRun checks\e[0m\n" | tee --append $INSTALL_LOG
    if [[ ! "$SKIP_TESTS" == "true"  ]]; then
        if [[ ("$BUILT_WITH_CLANG" == "true" && "$RECOMPILE" == "true") || (! "$BUILT_WITH_CLANG" == "true" && "$RECOMPILE" == "false" )]] ; then
            make -j ${CPUS} check-all 2>&1 | tee --append $INSTALL_LOG #; test ${PIPESTATUS[0]} -eq 0
        fi
    fi

    echo -e "\n\n\n\e[32mInstall\e[0m\n" | tee --append $INSTALL_LOG
    make install 2>&1 | tee --append $INSTALL_LOG ; test ${PIPESTATUS[0]} -eq 0

    if [[ "$RECOMPILE" == "true" ]]; then
        echo -e "\n\n\n\e[32mInitial installation has been finished\e[0m\n" | tee --append $INSTALL_LOG
        echo -e "\n\n\n\e[32mRebuild LLVM with clang\e[0m\n" | tee --append $INSTALL_LOG
        rm -rf $LLVM_BUILD
        mkdir -p $LLVM_BUILD
        cd $LLVM_BUILD

        INSTALLDIR=$INSTALLDIR_FINAL
        CC=$INSTALLDIR_TMP/bin/clang
        CXX=$INSTALLDIR_TMP/bin/clang++
        LD_LIBRARY_PATH=$INSTALLDIR_TMP/lib:$LD_LIBRARY_PATH
        CPATH=$INSTALLDIR_TMP/include:$CPATH

        RECOMPILE=false
        BUILT_WITH_CLANG=true
    else
        echo -e "\n\n\n\e[32mInstallation has been finished\e[0m\n" | tee --append $INSTALL_LOG
        break
    fi
done

# build a module file to use with modulesystem on the Lichtenberg

cat > $MODULEFILE << '_EOF'
-- Help
help([[
]])

-- Local variables
local ModuleFile  	= myFileName()
local FullModuleName    = myModuleFullName()
local ModuleDir         = string.gsub(ModuleFile, FullModuleName, "")
local ModuleDir         = string.gsub(ModuleDir, "/.lua", "")
local AppsDir		= string.gsub(ModuleDir, "${INSTALLDIR_FINAL}")
local ModuleRoot        = pathJoin(AppsDir,myModuleName(),myModuleVersion())
local use1,use2,use3    = string.match(ModuleFile, "^(.-)/([^/]+)/([^/]-)\.[^/\.]-\.lua$")
local UsePath           = pathJoin(use1, '.'..use2, use3)

-- Whatis
whatis("The LLVM Project is a collection of modular and reusable compiler and toolchain technologies. ")
whatis("URL: https://llvm.org/")
whatis("Keywords: ")
whatis("Full module name: "..FullModuleName)
whatis("Modules Dir: "..ModuleDir)
whatis("Module File: "..ModuleFile)
whatis("Apps Dir: "..AppsDir)
whatis("Application Root: "..ModuleRoot)

-- Dependencies
family("compiler")
-- depends_on()
-- prereq()

-- Environment
prepend_path("PATH", pathJoin(ModuleRoot, "bin"))
prepend_path("CPATH", pathJoin(ModuleRoot, "include"))
prepend_path("LIBRARY_PATH", pathJoin(ModuleRoot, "lib64"))
prepend_path("LD_LIBRARY_PATH", pathJoin(ModuleRoot, "lib64"))
-- prepend_path("MANPATH", pathJoin(ModuleRoot, "share/man"))
-- prepend_path("INFOPATH", pathJoin(ModuleRoot, "share/info"))

prepend_path("MODULEPATH", UsePath)
pushenv(myModuleName():upper().."_ROOT", ModuleRoot)

local io = require('io')
if (mode() == "load")
then
        pushenv("CC", io.popen("which clang"):read("*all"))
        pushenv("CXX", io.popen("which clang++"):read("*all"))
end

if (mode() == "unload")
then
        pushenv("CC",false)
        pushenv("CXX",false)
end

-- Messages
LmodMessage("Lmod: ", mode().."ing", myModuleName(), myModuleVersion())
'_EOF'

