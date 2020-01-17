#include "config.hpp"
#include <Foundation/Foundation.h>

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
