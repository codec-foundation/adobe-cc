cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

# ide layout
set(CMAKE_FOLDER foundation/plugin)

if (APPLE)
    # This is a relative path for CPack's staging directory - no leading /
    set (Foundation_CC_PLUGIN_INSTALL_DESTINATION "Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore")
else ()
    set (Foundation_CC_PLUGIN_INSTALL_DESTINATION .)
endif ()

# plugins

add_definitions(-DCODEC_NAME="${Foundation_CODEC_NAME}")

add_subdirectory(adobe_shared)
add_subdirectory(after_effects)
add_subdirectory(premiere)
