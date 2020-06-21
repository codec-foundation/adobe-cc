#pragma once

#include <string>
#ifdef WIN32
// <filesystem> not available on macOS < 10.15
#include <filesystem>
#endif

#ifdef __APPLE__
#include <boost/filesystem.hpp>
namespace fs = ::boost::filesystem;
#else
#include <filesystem>
namespace fs = ::std::filesystem;
#endif

namespace fdn
{

typedef fs::path PathType;
PathType getConfigurationPath();

}
