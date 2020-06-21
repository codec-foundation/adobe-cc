cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)
project(CodecFoundationSession)

# ide layout
set(CMAKE_FOLDER foundation/session)

if (APPLE)
    set(Boost_USE_STATIC_LIBS ON CACHE BOOL "Always use boost static libs" FORCE)
    set(Boost_USE_MULTITHREADED TRUE CACHE BOOL "Use boost multithreaded" FORCE)
    find_package(Boost 1.72 REQUIRED COMPONENTS filesystem)
endif(APPLE)

# this is the one we will install
add_library(CodecFoundationSession)

target_sources(CodecFoundationSession
    PUBLIC
        exporter.hpp
        freelist.hpp
        ffmpeg_helpers.hpp
        importer.hpp
        movie_reader.hpp
        movie_writer.hpp
        sample_cache.hpp
    PRIVATE
        exporter.cpp
        ffmpeg_helpers.cpp
        importer.cpp
        movie_reader.cpp
        movie_writer.cpp
)

target_link_libraries(CodecFoundationSession
    PUBLIC
        $<$<PLATFORM_ID:Darwin>:Boost::filesystem>
        ffmpeg::libavformat
        ffmpeg::libavcodec
        ffmpeg::libavutil        
    PRIVATE
        CodecRegistration
        Codec
	# need this for ffmpeg on windows; TODO: get rid of it
        $<$<PLATFORM_ID:Windows>:bcrypt>
)

# Include the ffmpeg headers
target_include_directories(CodecFoundationSession
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        ${ffmpeg_INCLUDE_DIRS}
    PRIVATE
)
