#pragma once

#include <nlohmann/json.hpp>

#include "platform_paths.hpp"

// read / write configuration to a suitable location

// configuration is always json information.

// clients are responsible for
//   - providing their own serialization <-> json
//   - handling missing configuration information

namespace fdn
{

using json = nlohmann::json;
const json &config(); // singleton access

}
