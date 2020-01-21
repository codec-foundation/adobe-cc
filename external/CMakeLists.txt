add_subdirectory(adobe)
add_subdirectory(ffmpeg)
add_subdirectory(googletest)

if (WIN32)
    add_subdirectory(StackWalker)
endif ()

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)
