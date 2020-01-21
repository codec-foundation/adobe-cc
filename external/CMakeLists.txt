add_subdirectory(adobe)
add_subdirectory(ffmpeg)

set( gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll" FORCE)
add_subdirectory(googletest)

if (WIN32)
    add_subdirectory(StackWalker)
endif ()

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)
