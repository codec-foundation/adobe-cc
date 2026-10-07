#pragma once

#include <string>
#include <filesystem>
namespace fs = ::std::filesystem;

namespace fdn
{

typedef fs::path PathType;
PathType getConfigurationPath();

}
