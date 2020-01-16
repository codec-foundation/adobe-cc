#include <fstream>
#include <iostream>

#ifdef WIN32
#include <ShlObj.h>
#endif

#include "config.hpp"

const std::string kConfigFilename{"config.json"};

namespace fdn
{

#ifdef WIN32
const PathType codecPath()
{
    PathType path;
    PWSTR programs;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramFilesX64, 0, NULL, &programs);
    if (SUCCEEDED(hr))
    {
        path = programs;
        path = path / "Adobe" / "Common" / "Plug-Ins" / "7.0" / "MediaCore" / FOUNDATION_CODEC_NAME;
        CoTaskMemFree(programs);
        return path;
    }
    throw std::runtime_error("could not get program files path");
}
#endif

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
        auto path = codecPath() / kConfigFilename;
#else
        auto path = codecPath() + "/" + kConfigFilename;
#endif
        std::ifstream in(path);
        if(!in.is_open())
        {
            throw std::runtime_error((std::string("could not open ") + path.string()).c_str());
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
