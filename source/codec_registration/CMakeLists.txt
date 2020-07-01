cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(CodecRegistration)

set (CMAKE_CXX_STANDARD 17)

# ide layout
set(CMAKE_FOLDER foundation/codec_registration)

if (APPLE)
    set(Boost_USE_STATIC_LIBS ON CACHE BOOL "Always use boost static libs" FORCE)
    find_package(Boost 1.72 REQUIRED COMPONENTS filesystem)
    find_library(Foundation Foundation)
endif (APPLE)

# this is the one we will install
add_library(CodecRegistration
    codec_registration.hpp
    config.cpp
    config.hpp
    logging.cpp
    logging.hpp
    util.cpp
    util.hpp
     $<$<PLATFORM_ID:Windows>:platform_paths_windows.cpp>
    platform_paths.hpp
     $<$<PLATFORM_ID:Darwin>:platform_paths_mac.mm>
)

# !!! We are dependent on this so the configuration file location gets compiled in
# !!! ideally the hosting environment would pass in this information

# used so we can look up install location from code, name log files, label dialog boxes
target_compile_definitions(CodecRegistration PUBLIC "FOUNDATION_CODEC_NAME=\"${Foundation_CODEC_NAME}\"")

target_link_libraries(CodecRegistration
    PUBLIC
        $<$<PLATFORM_ID:Darwin>:Boost::boost>
        nlohmann_json::nlohmann_json
    PRIVATE
        $<$<PLATFORM_ID:Windows>:StackWalker>
        $<$<PLATFORM_ID:Darwin>:${Foundation}>
)

target_include_directories(CodecRegistration
    PRIVATE
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/
)
