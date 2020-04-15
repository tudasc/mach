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
)

# if one wants to use mpi
#find_package(MPI REQUIRED)
#target_link_libraries(mpi_assertion_checker PRIVATE MPI::MPI_C)

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(mpi_assertion_checker PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(mpi_assertion_checker PROPERTIES
    COMPILE_FLAGS "-fno-rtti -Wall -Wextra -Wno-unused-parameter -g"
)

