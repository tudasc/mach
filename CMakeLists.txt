cmake_minimum_required(VERSION 3.16)# what I use on the cluster, may be set lower

project("MPI Assertion Checking")

find_package(LLVM REQUIRED CONFIG)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

add_subdirectory(dump_ir_pass)
add_subdirectory(mpi_assertion_checker)
