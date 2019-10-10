#include "presets.hpp"
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
        // Create the directory if it doesn't already exist
        BOOL exists = [[NSFileManager defaultManager] fileExistsAtPath:[dst path]];
        if (!exists)
        {
            exists = [[NSFileManager defaultManager] createDirectoryAtURL:dst withIntermediateDirectories:NO attributes:nil error:nil];
        }
        // If the directory exists (or we successfully created it)
        if (exists)
        {
            dst = [dst URLByAppendingPathComponent:[NSString stringWithUTF8String:file.c_str()]];
            exists = [[NSFileManager defaultManager] fileExistsAtPath:[dst path]];
            // If the file exists and we are to replace it, delete it
            if (replace && exists)
            {
                exists = ![[NSFileManager defaultManager] removeItemAtURL:dst error:nil];
            }
        }
        // If the file does not exist
        if (!exists)
        {
            // Ignoring result of this for now
            [[NSFileManager defaultManager] copyItemAtURL:source toURL:dst error:nil];
        }
    }
}
