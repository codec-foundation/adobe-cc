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
    add_compile_options(-msse4.1)
endif()

# library dependencies
# !!! already included - add_subdirectory(external)

option(Foundation_PACKAGE_TESTS "Build the tests" ON)

# documentation
# add_subdirectory(doc)

# plugins
add_subdirectory(source)


