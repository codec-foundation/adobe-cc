#include "presets.hpp"
#include <cctype>
#include <filesystem>
#include <Foundation/Foundation.h>

std::vector<Presets::PathType> Presets::getDestinationDirectoryPaths()
{
    std::vector<PathType> paths;
    @autoreleasepool {
        NSArray *urls = [[NSFileManager defaultManager] URLsForDirectory:NSLibraryDirectory inDomains:NSLocalDomainMask];
        for (NSURL *url in urls)
        {
            NSURL *prefs = [url URLByAppendingPathComponent:@"Preferences" isDirectory:YES];
            prefs = [prefs URLByAppendingPathComponent:@"com.Adobe.Premiere Pro.paths.plist" isDirectory:NO];
            NSDictionary<NSString *, id> *dict = [NSDictionary dictionaryWithContentsOfURL:prefs];
            if ([dict isKindOfClass:[NSDictionary class]])
            {
                for (NSString *key in dict)
                {
                    NSDictionary *version = dict[key];
                    if ([version isKindOfClass:[NSDictionary class]])
                    {
                        NSString *path = version[@"CommonExporterPresetsPath"];
                        if ([path isKindOfClass:[NSString class]])
                        {
                            paths.push_back([[path stringByExpandingTildeInPath] cStringUsingEncoding:NSUTF8StringEncoding]);
                        }
                    }
                }
            }
        }

        // Current Media Encoder (2020+) keeps user presets in ~/Documents/Adobe/Adobe Media Encoder/<version>/Presets,
        // as on Windows; the paths.plist above no longer exists on these installs.
        NSArray *docs = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *documents = [docs firstObject];
        if (documents)
        {
            std::filesystem::path ame([documents fileSystemRepresentation]);
            ame = ame / "Adobe" / "Adobe Media Encoder";
            std::error_code error;
            if (std::filesystem::is_directory(ame, error))
            {
                for (const auto& entry : std::filesystem::directory_iterator(ame, error))
                {
                    const auto name = entry.path().filename().string();
                    // version folders only ("26.0"), not e.g. "Adobe Adobe Media Encoder Audio Previews"
                    if (entry.is_directory(error) && !name.empty() && std::isdigit(static_cast<unsigned char>(name[0])))
                    {
                        paths.push_back((entry.path() / "Presets").string());   // PathType is std::string on macOS
                    }
                }
            }
        }
    }
    return paths;
}

static NSURL *getSourceDirectoryURL()
{
    NSBundle *bundle = [NSBundle bundleWithIdentifier:FOUNDATION_MACOSX_BUNDLE_GUI_IDENTIFIER];
    return [bundle URLForResource:@"Presets" withExtension:nil];
}

Presets::PathType Presets::getSourceDirectoryPath()
{
    @autoreleasepool {
        return [[getSourceDirectoryURL() path] cStringUsingEncoding:NSUTF8StringEncoding];
    }
}

std::vector<Presets::PathType> Presets::getPresetFileNames()
{
    std::vector<Presets::PathType> names;
    @autoreleasepool {
        NSURL *source = getSourceDirectoryURL();
        if (source)
        {
            NSArray<NSURL *> *contents = [[NSFileManager defaultManager] contentsOfDirectoryAtURL:source includingPropertiesForKeys:nil options:NSDirectoryEnumerationSkipsHiddenFiles error:nil];
            for (NSURL *url in contents)
            {
                if ([[url pathExtension] isEqualToString:@"epr"])
                {
                    names.push_back([[url lastPathComponent] cStringUsingEncoding:NSUTF8StringEncoding]);
                }
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
    @autoreleasepool {
        return [[NSFileManager defaultManager] fileExistsAtPath:[NSString stringWithUTF8String:directory.c_str()]] ? true : false;
    }
}

bool Presets::createDirectory(const Presets::PathType &directory)
{
    if (directory.empty())
    {
        return false;
    }
    @autoreleasepool {
        NSURL *dir = [NSURL fileURLWithPath:[NSString stringWithUTF8String:directory.c_str()] isDirectory:YES];
        return [[NSFileManager defaultManager] createDirectoryAtURL:dir
                                        withIntermediateDirectories:NO
                                                         attributes:nil
                                                              error:nil] ? true : false;
    }
}

void Presets::copy(const Presets::PathType &file, const Presets::PathType &source_dir, const Presets::PathType &destination_dir, bool replace)
{
    if (source_dir.empty() || destination_dir.empty() || file.empty())
    {
        return;
    }
    @autoreleasepool {
        NSURL *source = [NSURL fileURLWithPath:[NSString stringWithUTF8String:source_dir.c_str()] isDirectory:YES];
        source = [source URLByAppendingPathComponent:[NSString stringWithUTF8String:file.c_str()]];
        NSURL *dst = [NSURL fileURLWithPath:[NSString stringWithUTF8String:destination_dir.c_str()] isDirectory:YES];
        dst = [dst URLByAppendingPathComponent:[NSString stringWithUTF8String:file.c_str()]];
        BOOL exists = [[NSFileManager defaultManager] fileExistsAtPath:[dst path]];
        // If the file exists and we are to replace it, delete it
        if (replace && exists)
        {
            exists = ![[NSFileManager defaultManager] removeItemAtURL:dst error:nil];
        }
        // If the file does not exist
        if (!exists)
        {
            // Ignoring result of this for now
            [[NSFileManager defaultManager] copyItemAtURL:source toURL:dst error:nil];
        }
    }
}
