#pragma once

#include <string>
#ifdef WIN32
// <filesystem> not available on macOS < 10.15
#include <filesystem>
#endif

namespace fdn
{

#ifndef __APPLE__
typedef std::filesystem::path PathType;
#else
// using std::filesystem::path introduces a requirement for macOS > 10.15
// !!! revisit when support for < 10.15 is dropped
typedef std::string PathType;
#endif
PathType getConfigurationPath();

}
