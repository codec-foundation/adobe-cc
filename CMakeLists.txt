cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

# ide layout
set(CMAKE_FOLDER foundation)

# modules

# project

project(CodecFoundation)

set (CMAKE_CXX_STANDARD 17)

if(MSVC)
    # add_compile_options(/arch:AVX2)
    # may want to use this for specific source files
else()
    # x86 slices only - lets a universal (arm64 + x86_64) macOS build compile
    add_compile_options("SHELL:-Xarch_x86_64 -msse4.1")
endif()

# library dependencies
# !!! already included - add_subdirectory(external)

option(Foundation_PACKAGE_TESTS "Build the tests" ON)

# documentation
# add_subdirectory(doc)

# plugins
add_subdirectory(source)


