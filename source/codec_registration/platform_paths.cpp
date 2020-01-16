#include <ShlObj.h>

#include "platform_paths.hpp"

namespace fdn {

const PathType getConfigurationPath()
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

} // namespace fdn