#pragma once
#include <vector>
#include <string>
#ifdef WIN32
// <filesystem> not available on macOS < 10.15
#include <filesystem>
#endif

namespace Presets {
#ifdef WIN32
	using PathType = std::filesystem::path;
#else
	using PathType = std::string;
#endif
    std::vector<PathType> getDestinationDirectoryPaths();
    PathType getSourceDirectoryPath();
    std::vector<PathType> getPresetFileNames();
    bool directoryExists(const PathType &directory);
    bool createDirectory(const PathType &directory);
    void copy(const PathType &file, const PathType &source_dir, const PathType &destination_dir, bool replace);
}
