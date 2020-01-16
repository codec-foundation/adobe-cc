#pragma once

#ifdef WIN32
// <filesystem> not available on macOS < 10.15
#include <filesystem>
#endif

#include <nlohmann/json.hpp>

// read / write configuration to a suitable location

// configuration is always json information.

// clients are responsible for
//   - providing their own serialization <-> json
//   - handling missing configuration information

namespace fdn
{

using json = nlohmann::json;

#ifdef WIN32
typedef std::filesystem::path PathType;
#else
typedef std::string PathType;
#endif

const PathType codecPath();
const json &config(); // singleton access

}
