#include "presets.hpp"

#include "presets.hpp"
#include <Foundation/Foundation.h>

std::vector<std::string> Presets::getDestinationDirectoryPaths()
{
    std::vector<std::string> paths;
    // TODO: 
    return paths;
}

std::string Presets::getSourceDirectoryPath()
{
    // TODO:
    return std::string();
}

std::vector<std::string> Presets::getPresetFileNames()
{
    std::vector<std::string> names;
    // TODO: 
    return names;
}

void Presets::copy(const std::string &file, const std::string &source_dir, const std::string &destination_dir, bool replace)
{
    if (source_dir.empty() || destination_dir.empty() || file.empty())
    {
        return;
    }
    // TODO: 
}
