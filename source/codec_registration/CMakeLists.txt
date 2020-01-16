cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(CodecRegistration)

set (CMAKE_CXX_STANDARD 17)

# !!! We are dependent on this so the configuration file location gets compiled in
# !!! ideally the hosting environment would pass in this information
add_definitions(-DFOUNDATION_CODEC_NAME=\"${Foundation_CODEC_NAME}\")

# this is the one we will install
add_library(CodecRegistration
    codec_registration.hpp
    config.cpp
    config.hpp
    logging.cpp
    logging.hpp
    util.cpp
    util.hpp
)

target_link_libraries(CodecRegistration
    nlohmann_json::nlohmann_json
    StackWalker
)

target_include_directories(CodecRegistration
    PRIVATE
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/
)
