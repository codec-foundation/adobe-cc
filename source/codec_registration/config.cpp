#include <fstream>
#include <iostream>
#include <string>

#include "config.hpp"
#include "platform_paths.hpp"

using namespace std::string_literals;

const std::string kConfigFilename{"config.json"};

namespace fdn
{

static std::string defaultLoad()
{
    return R"({
    })";         //!!! might put metaconfig information here - where loaded from
                 //!!! checkout hash etc
}

static json load()
{
    json loaded;

    try {
#ifdef WIN32
        auto path = getConfigurationPath() / kConfigFilename;
#else
        auto path = getConfigurationPath() + "/" + kConfigFilename;
#endif
        std::ifstream in(path);
        if(!in.is_open())
        {
            throw std::runtime_error(("could not open "s +
#ifndef __APPLE__
                                      path.string()
#else
                                      path
                                      // !!! < 10.15 needs this
#endif
                                      ).c_str());
        }
        in >> loaded;
    } catch (...) {
        loaded = json::parse(defaultLoad());
    }
    return loaded;
}

const json& config()
{
    static json theConfig(load());

    return theConfig;
}

}
