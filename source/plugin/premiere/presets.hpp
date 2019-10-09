#pragma once
#include <vector>
#include <string>

namespace Presets {
    std::vector<std::string> getDestinationDirectoryPaths();
    std::string getSourceDirectoryPath();
    std::vector<std::string> getPresetFileNames();
    void copy(const std::string &file, const std::string &source_dir, const std::string &destination_dir, bool replace);
}
