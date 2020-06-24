cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)
project(PremierePlugin)

# this is the one we will install
add_library(PremierePlugin MODULE)

add_definitions(-DFOUNDATION_FILE_IMPORT_FOUR_CC={Foundation_FILE_IMPORT_FOUR_CC})

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
        presets.hpp
        $<$<PLATFORM_ID:Windows>:presets.cpp>
        $<$<PLATFORM_ID:Darwin>:presets.mm>
)

find_library(Foundation Foundation)

target_link_libraries(PremierePlugin
    PRIVATE
        Codec
        CodecRegistration
        CodecFoundationSession
        AdobePremiereSdk
        AdobeShared
        $<$<PLATFORM_ID:Darwin>:${Foundation}>
)

if (MSVC)
    set_target_properties(PremierePlugin
        PROPERTIES
            OUTPUT_NAME "${Foundation_CODEC_NAME}Plugin"
            SUFFIX ".prm"
    )
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION bin OPTIONAL)
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

# rn Preset install to plugin is macOS-only
if (APPLE)
    if (DEFINED Foundation_PRESETS)
        # Install presets to Resources/Presets
        target_sources(PremierePlugin
            PRIVATE
                ${Foundation_PRESETS}
        )
        set_source_files_properties(
            ${Foundation_PRESETS}
            PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/Presets"
        )
        # See CMakeLists.txt in installer for script to handle install to CC locations
    endif ()
endif (APPLE)

set_target_properties(PremierePlugin
    PROPERTIES
	    BUNDLE TRUE
	    MACOSX_BUNDLE_GUI_IDENTIFIER ${Foundation_IDENTIFIER_PREFIX}.pr
)

if (APPLE)
    # used so we can look up bundle location from code
    set_property(SOURCE presets.mm
                    APPEND
                    PROPERTY COMPILE_DEFINITIONS FOUNDATION_MACOSX_BUNDLE_GUI_IDENTIFIER=@\"${Foundation_IDENTIFIER_PREFIX}.pr\"
    )
endif (APPLE)
install(
  TARGETS PremierePlugin
  RUNTIME
  BUNDLE DESTINATION ${Foundation_CC_PLUGIN_INSTALL_DESTINATION}
  LIBRARY DESTINATION ${Foundation_CC_PLUGIN_INSTALL_DESTINATION}
  COMPONENT plugin
)
