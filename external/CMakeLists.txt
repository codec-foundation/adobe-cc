set_property(GLOBAL PROPERTY CMAKE_FOLDER external)

# ide layout
set(CMAKE_FOLDER foundation/external)

# external code

add_subdirectory(adobe)
add_subdirectory(ffmpeg)

# google tests framework
set( gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll" FORCE)
add_subdirectory(googletest)

if (WIN32)
    add_subdirectory(StackWalker)
endif ()

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)
