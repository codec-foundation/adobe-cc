cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# presets asset files
# win64 installer
#   as of publication, details of where to place installed files comes
#   from emailed advice; SDK docs are for previous versions.
#
#   Win64
#       Nov 17 CC SDK reads:
#
#       look at registry 
#           Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Adobe\Premiere Pro\12.0
#           Key:CommonExporterPresetsPath
#       which will say
#           [User folder]\AppData\Roaming\Adobe\Common\AME\[version]\Presets\
#
#       but this is out of date, and current advice is to put them in
#           C:\Users\[USERID]\Documents\Adobe\Adobe Media Encoder\[MAJOR_VERSION.0]\Preseets
#
#   TODO: update to use registry keys once this is possible
#
# set CPACK_PACKAGE_* in codec-specific installer CMakeLists.txt
# set CPACK_RESOURCE_FILE_LICENSE in codec-specific installer. Please include foundation license.txt contents within.
# set CPACK_NSIS_* in codec-specific installer
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

# Presets are their own component on Windows, only if present in the project
if (MSVC AND DEFINED Foundation_PRESETS)
    set(CPACK_COMPONENTS_ALL plugin Presets ${Foundation_CUSTOM_INSTALLER_COMPONENTS})
else ()
    set(CPACK_COMPONENTS_ALL plugin ${Foundation_CUSTOM_INSTALLER_COMPONENTS})
endif ()

# macOS installer specific settings
if (APPLE)
    set (CPACK_GENERATOR productbuild)
    set (CPACK_PACKAGING_INSTALL_PREFIX "/")
    if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
       set(CMAKE_INSTALL_PREFIX "/" CACHE PATH "Install path prefix, prepended onto install directories." FORCE)
    endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if (NOT CPACK_RESOURCE_FILE_README)
        message(AUTHOR_WARNING "CPACK_RESOURCE_FILE_README is not set. CMake's default will be used.")
    endif ()
    if (NOT CPACK_RESOURCE_FILE_WELCOME)
        set (CPACK_RESOURCE_FILE_WELCOME ${CMAKE_CURRENT_LIST_DIR}/Welcome.txt)
    endif ()
    # User can opt to provide a different name outside this file
    if (NOT CPACK_PRODUCTBUILD_IDENTITY_NAME)
        set (CPACK_PRODUCTBUILD_IDENTITY_NAME "Developer ID Installer")
    endif ()
    
    # Default is Darwin, not useful for generated file names:
    set (CPACK_SYSTEM_NAME macOS)
endif (APPLE)
# NSIS component specific settings
set(CPACK_NSIS_plugin_INSTALL_DIRECTORY "$PROGRAMFILES64\\\\Adobe\\\\Common\\\\Plug-ins\\\\7.0\\\\MediaCore\\\\${Foundation_CODEC_NAME}")
# set(CPACK_INSTALL_COMMANDS, "magic command to get plugin directory and copy to it")
# currently doing this by overriding template
# TODO: don't override template
# override modules path so we can change the nsis template
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_MODULE_PATH})

# presets asset files

if (DEFINED Foundation_PRESETS)
    if (APPLE)
        set (Foundation_CC_PLUGIN_INSTALL_DESTINATION "Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore")
        # Configure postinstall script to install Presets for CC
        set(Foundation_PRESETS_INSTALL_SRC
            "/${Foundation_CC_PLUGIN_INSTALL_DESTINATION}/${Foundation_CODEC_NAME}PremierePlugin.bundle/Contents/Resources/Presets/")
        configure_file(postinstall.in ${CMAKE_CURRENT_BINARY_DIR}/postinstall)
        set(CPACK_POSTFLIGHT_PLUGIN_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/postinstall")
    else ()
        install(
            FILES
                ${Foundation_PRESETS}
            DESTINATION
                "Presets"
            COMPONENT
                Presets
        )
    endif ()
else ()
    message(NOTICE "Foundation_PRESETS is not defined, no presets will be installed")
endif ()

# codec specific component install paths
set(CPACK_NSIS_Presets_INSTALL_DIRECTORY "$PROFILE\\\\Documents\\\\Adobe\\\\Adobe\ Media\ Encoder\\\\13.0")

# generic installer framework
# This must always be after CPACK settings
include(CPack)
# components to include to installer
cpack_add_component(plugin
    DISPLAY_NAME "Plugin"
    DESCRIPTION "plugin module"
    REQUIRED
)
if (DEFINED Foundation_PRESETS)
    cpack_add_component(Presets
        DISPLAY_NAME "Presets"
        DESCRIPTION "Presets"
    )
endif ()

