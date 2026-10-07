cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(ffmpeg)

# could put a build step here
# ExternalProject_Add(
#    PREFIX
#        FFmpeg
#
#)

# FFMPEG_ROOT: an installed FFmpeg prefix (lib/ + include/), e.g. a lipo'd universal macOS build.
# Left empty, the in-tree build of the FFmpeg submodule is used as before.
set(FFMPEG_ROOT "" CACHE PATH "Installed FFmpeg prefix; empty uses the in-tree submodule build")
if(FFMPEG_ROOT)
    set(FFMPEG_INCLUDE_DIR ${FFMPEG_ROOT}/include)
    foreach(lib libavcodec libavformat libavutil)
        set(FFMPEG_LIB_${lib} ${FFMPEG_ROOT}/lib/${lib}.a)
    endforeach()
else()
    set(FFMPEG_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/FFmpeg)
    foreach(lib libavcodec libavformat libavutil)
        set(FFMPEG_LIB_${lib} ${CMAKE_CURRENT_LIST_DIR}/FFmpeg/${lib}/${lib}.a)
    endforeach()
endif()

add_library(libavcodec STATIC IMPORTED GLOBAL)
add_library(libavformat STATIC IMPORTED GLOBAL)
add_library(libavutil STATIC IMPORTED GLOBAL)

set_target_properties(libavcodec
    PROPERTIES
        IMPORTED_LOCATION ${FFMPEG_LIB_libavcodec}
)
set_target_properties(libavformat
    PROPERTIES
        IMPORTED_LOCATION ${FFMPEG_LIB_libavformat}
)
set_target_properties(libavutil
    PROPERTIES
        IMPORTED_LOCATION ${FFMPEG_LIB_libavutil}
)

find_library(CoreVideo CoreVideo)

target_link_libraries(libavutil
    INTERFACE $<$<PLATFORM_ID:Darwin>:${CoreVideo}>
)

target_include_directories(libavcodec INTERFACE
    ${FFMPEG_INCLUDE_DIR}
)
target_include_directories(libavformat INTERFACE
    ${FFMPEG_INCLUDE_DIR}
)
target_include_directories(libavutil INTERFACE
    ${FFMPEG_INCLUDE_DIR}
)

add_library(ffmpeg::libavcodec ALIAS libavcodec)
add_library(ffmpeg::libavformat ALIAS libavformat)
add_library(ffmpeg::libavutil ALIAS libavutil)

# add_include(ffmpeg ${CMAKE_CURRENT_LIST_DIR}/FFmpeg)
