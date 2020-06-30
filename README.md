# Codec Foundation for Adobe CC

This is the community-supplied foundation for creating plugins to Adobe CC 2018 and 2019.

Development of this plugin was sponsored by
 - [disguise](http://disguise.one), makers of the disguise show production software and hardware.
 - [10bitFX](http://notch.one), creators of the Notch VFX software

Principal contributors to this plugin are

-  Greg Bakker (gbakker@gmail.com)
-  Richard Sykes
-  [Tom Butterworth](http://kriss.cx/tom)
-  [Nick Zinovenko](https://github.com/exscriber)

Thanks to Tom Butterworth for creating the Hap codec and Vidvox for supporting that development.

Please see [license.txt](license.txt) for the licenses of this plugin and the components used to create it.

# Development

The following information is for developers who wish to contribute to the project.

## Prerequisites

### compiler toolchain

You'll need a compiler environment appropriate to your operating system. The current plugin has been developed on

-  win64 with Microsoft Visual Studio 2017 Professional.
-  macOS with Xcode

### Submodules

Populate FFmpeg and other submodules with

    git submodule update --init --recursive

### CMake

CMake creates the build system for the supported target platforms. This project requires version 3.15.0 or later.

[https://cmake.org/install/](https://cmake.org/install/)

### NSIS

NSIS is required for win32 installer builds.

[http://nsis.sourceforge.net/Main_Page](http://nsis.sourceforge.net/Main_Page)

### Adobe CC 2019 SDKs

The following SDKs are required from Adobe.

<https://console.adobe.io/downloads>

| SDK           | Location                       |
|---------------|--------------------------------|
| Premiere      | external/adobe/premiere        |
| After Effects | external/adobe/AfterEffectsSDK |

### FFMpeg

FFmpeg 4.0 is used for output of the .mov format.

The FFMpeg build is not wrapped by the plugin's cmake build process, and must be made in a platform specific process as descibed below.

#### win64

Either install and set environment for your own FFMpeg, or build / install the one in external/ffmpeg as described at

<https://trac.ffmpeg.org/wiki/CompilationGuide/MSVC>

For reference, the FFMpeg build for the win64 plugin was created by

-  first installing [MSYS](http://www.mingw.org/wiki/msys)

-  launching a Visual Studio 2017 developer prompt
-  set Visual Studio build vars for an x64 build. Something like

        "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat" amd64

-  running the MSYS shell from within that prompt

        C:\MinGW\msys\1.0\msys.bat
 
-  going to the external/ffmpeg/FFMPeg directory and then

        ./configure --toolchain=msvc --disable-x86asm --disable-network --disable-everything --enable-muxer=mov --enable-demuxer=mov --extra-cflags="-MD -arch:AVX"
        make

This will take a while.

#### macOS

Build a local FFmpeg by opening a terminal and moving to external/ffmpeg/FFmpeg. Then

    ./configure --disable-x86asm --disable-network --disable-everything --enable-muxer=mov --enable-demuxer=mov --disable-zlib --disable-iconv
    make

### macOS specific

For macos, the Boost library is needed for missing std::filesystem support in Mojave and earlier. The homebrew installation is recommended (requires [Homebrew](https://brew.sh)).

    brew install boost

##  Building

### win64

First create a build directory at the top level, and move into it

    mkdir build
    cd build

Invoke cmake to create a Visual Studio .sln

    cmake -DCMAKE_GENERATOR_PLATFORM=x64 ..

This should create HapEncoder.sln in the current directory. Open it in Visual Studio:

    HapEncoder.sln

The encoder plugin (.prm) is created by building all.
The installer executable is made by building the PACKAGE target, which is excluded from the regular build.

### macOS

First create a build directory at the top level, and move into it

    mkdir Release
    cd Release

Invoke cmake to create makefiles etc for a Release build

    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64 ..

Then do a build with

    make -j

Alternatively to create an Xcode project

    cmake -DCMAKE_OSX_ARCHITECTURES=x86_64 -G Xcode ..

Then open the generated Xcode project.

This will create a plugin at

    Release/source/premiere_CC2018/HapEncoderPlugin.bundle

To create an installer (requires Apple Developer Program membership for signing)

    cpack

To create an unsigned installer for development testing (do not distribute unsigned installers)

    cpack -D CPACK_PRODUCTBUILD_IDENTITY_NAME=

The installer is created in the Release directory.

#### Notarizing macOS Installers

Installers for macOS **must** be notarized by Apple. If you skip this step, users will not be able to install your software. Because it requires prior setup it is not automated. Follow the instructions under "Upload Your App to the Notarization Service" in Apple's [Customizing the Notarization Workflow](https://developer.apple.com/documentation/xcode/notarizing_your_app_before_distribution/customizing_the_notarization_workflow?language=objc) guide.

## Testing

The foundation uses the Google Tests framework.

Test targets are present in IDEs or test executables can be run directly from a commandline.

## Design

### Structure

The layout of a project using the foundation is

    .
    ├── CMakeLists.txt
    ├── README.md
    ├── <sources for your codec>
    └── external
        ├── foundation
        │   └── <foundation source
        └── <other external modules for your codec>


and the foundation layout is

    .
    ├── external
    │   ├── StackWalker
    │   │   <Stack trace functionality; windows only for now>
    │   ├── adobe
    │   │   │   <Adobe supplied SDKs must be dropped here>
    │   │   ├── AfterEffectsSDK
    │   │   └── premiere
    │   ├── ffmpeg
    │   │   <reading and writing to .mov container format>
    │   ├── googletest
    │   └── json
    │       <serialise configuration information for
    │        logging and for AfterFX ui>
    ├── installer
    └── source
        ├── codec_registration
        │   <interface that your library must implement,
        │    plus logging and configuration functionality>
        ├── plugin
        │   ├── adobe_shared
        │   │   <utilities shared between AfterFX and Premiere>  
        │   ├── after_effects
        │   │   <output module for AfterFX>
        │   │   ├── mac
        │   │   │   <mac UI implementation>
        │   │   └── win
        │   │       <windows ui implementation>
        │   └── premiere
        │       <premiere plugin used for import in Adobe Media
        │        Encoder, AfterFX and Premiere; and export in
        │        Adobe Media Encoder and Premiere>
        ├── session
        │   <wrappers for importing and exporting frame
        │    sequences, and reading / writing .mov files via
        │    ffmpeg>
        └── test
            <test harnesses>

The foundation expects the hosting project to expose its codec via the interfaces defined in codec_registration.

### Build configuration

In its top level CMakeLists, the host should set some codec-specific information to be included throughout the build:

| Variable                                     | Optional | Example                                      | Purpose     |
| -------------------------------------------- | -------- | -------------------------------------------- | ----------- |
| Foundation_IDENTIFIER_PREFIX                 | No       | org.YourOrganisationName.YourCodecPluginName |             |
| Foundation_CODEC_NAME                        | No       | YourCodec                                    |             |
| Foundation_CodecSpecificComponents           | Yes      |                                              |             |
| Foundation_FILE_IMPORT_FOUR_CC               | No       | 0x1234ABCDL                                  | unique importer id so After Effects picks plugin up as an importer |
| Foundation_CODEC_NAME_WITH_HEX_SIZE_PREFIX   | No       | "\\x07MODTHIS"                               | unique id needed for premier plugin. Size must be 0x7 atm, and padded to 7-bytes- we're not using the recommended resource builder step that compiles the correct sizes around this            |
| Foundation_PRESETS                           | Yes      | list of files                                |             |
| Foundation_PACKAGE_TESTS                     | Yes      | FALSE                                        | build or don't build tests |

### Plugin Configuration

The codec_registration library provides a means of obtaining configuration information both for itself and for your own codecs.

Configuration information is stored in json format, and on windows is loaded from

    C:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore\<your plugin name>\config.json

and on macos is loaded from

    /Users/yourusername/Library/Application Support/CodecFoundation/config.json

a sample config.json file on macos is

    {
        "logging": {
            "threshold": "debug",
            "path": "/Users/yourname/Desktop"
        },
        "exporter": {
           "initialWorkers": 1,
           "maxWorkers": -1
        }
    }

meaning that maximum logging is enabled, the exporters start with 1 worker thread and use a maximum of <number of cores on your machine> workers.

Your own configuration may be added alongside these. It is available as parsed json, from which you can serialise. Please see external/json for details.

Assuming you have implemented an nlohmann::json serializer for your configuration information, you could obtain it in your plugin with

    #include "config.hpp"

    ...

    YourConfiguration config;
    fdn::config().at("exporter").get_to(config);
    
### Logging

The foundation also supplies a thread-safe logging facility that you may use in your own code. This is available in the codec_registration library.

Please see logging.hpp for details, but after #including config.hpp you may use the various FDN_<log level> macros.

    FDN_INFO("we are now doing this", b, "something", 17.0);
    FDN_INFO("we are now doing that", d);

    FDN_DEBUG("testvar7:", testvar7);
    FDN_WARN("disk space seems low");
    FDN_ERROR("ran out of disk space");
    FDN_FATAL("heap corruption detected; exiting");

Logging information is written to the debug output of an attached debugger, and additionally to a logfile as described in the Plugin Configuration section above.
