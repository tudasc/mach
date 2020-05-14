add_library(mpi_assertion_checker MODULE
    # List your source files here.
    mpi_assertion_checker.cpp
    mpi_functions.h
    mpi_functions.cpp
    conflict_detection.h
    conflict_detection.cpp
    implementation_specific.h
    implementation_specific.cpp
    function_coverage.h
    function_coverage.cpp
    additional_assertions.h
    additional_assertions.cpp
    debug.h
    analysis_result.h
)

# if one wants to use mpi
#find_package(MPI REQUIRED)
#target_link_libraries(mpi_assertion_checker PRIVATE MPI::MPI_C)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Og -DDEBUG_MACH_PASS=1")
# swith off one Warning for release build: some variables are only used for assertions
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DDEBUG_MACH_PASS=0 -Wno-unused-but-set-variable")

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(mpi_assertion_checker PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(mpi_assertion_checker PROPERTIES
    COMPILE_FLAGS "-fno-rtti -Wall -Wextra -Wno-unused-parameter"
)

