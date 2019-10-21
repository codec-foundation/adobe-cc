#include "presets.hpp"
#include <ShlObj.h>
#include <filesystem>

std::vector<Presets::PathType> Presets::getDestinationDirectoryPaths()
{
    std::vector<PathType> paths;
    /*
	Although Adobe SDK documentation suggests consulting registry keys for this,
	the information seems to be out of date.
	*/

	PWSTR documents;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &documents);
	if (SUCCEEDED(hr))
	{
		std::filesystem::path ame(documents);
		ame = ame / "Adobe" / "Adobe Media Encoder";
		std::error_code error;
		if (std::filesystem::exists(ame, error) && std::filesystem::is_directory(ame, error))
		{
			for (const auto& entry : std::filesystem::directory_iterator(ame, error))
			{
				if (entry.is_directory(error))
				{
					const auto path = entry.path() / "Presets";
					paths.push_back(path);
				}
			}
		}
		CoTaskMemFree(documents);
	}

    return paths;
}

Presets::PathType Presets::getSourceDirectoryPath()
{
	std::filesystem::path path;
	PWSTR programs;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramFilesX64, 0, NULL, &programs);
	if (SUCCEEDED(hr))
	{
		path = programs;
		path = path / "Adobe" / "Common" / "Plug-Ins" / "7.0" / "MediaCore";
		path /= FOUNDATION_CODEC_NAME;
		path /= "Presets";
		CoTaskMemFree(programs);
	}
    return path;
}

std::vector<Presets::PathType> Presets::getPresetFileNames()
{
    std::vector<Presets::PathType> names;
	const auto& source = getSourceDirectoryPath();
	std::error_code error;
	if (std::filesystem::exists(source, error) && std::filesystem::is_directory(source, error))
	{
		for (const auto& entry : std::filesystem::directory_iterator(source, error))
		{
			if (entry.is_regular_file(error) && entry.path().extension() == ".epr")
			{
				names.push_back(entry.path().filename());
			}
		}
	}
    return names;
}

bool Presets::directoryExists(const Presets::PathType &directory)
{
    if (directory.empty())
    {
        return false;
    }
	std::error_code error;
	return std::filesystem::exists(directory, error) && std::filesystem::is_directory(directory, error);
}

bool Presets::createDirectory(const Presets::PathType &directory)
{
    if (directory.empty())
    {
        return false;
    }
	std::error_code error;
	return std::filesystem::create_directory(directory, error);
}

void Presets::copy(const Presets::PathType& file, const Presets::PathType& source_dir, const Presets::PathType& destination_dir, bool replace)
{
	if (source_dir.empty() || destination_dir.empty() || file.empty())
	{
		return;
	}
	const auto& dest = destination_dir / file;
	const auto& src = source_dir / file;
	std::error_code error;
	if (replace || !std::filesystem::exists(dest, error))
	{
		// Copy the file
		std::filesystem::copy_file(src, dest, replace ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none, error);
		// Ignoring error for now
	}
}
