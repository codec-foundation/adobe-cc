cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)
project(PremierePlugin)

# this is the one we will install
add_library(PremierePlugin MODULE)

target_sources(PremierePlugin
    PRIVATE
        premiereImporter.cpp
        premiereImporter.hpp
        async_importer.cpp
        async_importer.hpp
        configure.hpp
        export_settings.cpp
        export_settings.hpp
        file_import.rc
        main.cpp
        main.hpp
        premiereParams.cpp
        premiereParams.hpp
        prstring.cpp
        prstring.hpp
        targetver.h
)

target_link_libraries(PremierePlugin
    PRIVATE
        Codec
        CodecRegistration
        CodecFoundationSession
        AdobePremiereSdk
        AdobeShared
)

if (MSVC)
    set_target_properties(PremierePlugin
        PROPERTIES
            OUTPUT_NAME "${Foundation_CODEC_NAME}Plugin"
            SUFFIX ".prm"
    )
elseif (APPLE)
# macOS plugins have a common extension, so identify the host in the name
    set_target_properties(PremierePlugin
        PROPERTIES
            OUTPUT_NAME "${Foundation_CODEC_NAME}PremierePlugin"
    )
endif()

if (NOT DEFINED Foundation_CC_PLUGIN_INSTALL_DESTINATION)
    message(FATAL_ERROR "Parent must define Foundation_CC_PLUGIN_INSTALL_DESTINATION")
endif ()
if (APPLE)
if (NOT DEFINED Foundation_IDENTIFIER_PREFIX)
message(FATAL_ERROR "Parent must define Foundation_IDENTIFIER_PREFIX")
endif ()
endif (APPLE)

set_target_properties(PremierePlugin
    PROPERTIES
	    BUNDLE TRUE
	    MACOSX_BUNDLE_GUI_IDENTIFIER ${Foundation_IDENTIFIER_PREFIX}.pr
)

install(
  TARGETS PremierePlugin
  RUNTIME
  BUNDLE DESTINATION ${Foundation_CC_PLUGIN_INSTALL_DESTINATION}
  LIBRARY DESTINATION ${Foundation_CC_PLUGIN_INSTALL_DESTINATION}
  COMPONENT plugin
)
