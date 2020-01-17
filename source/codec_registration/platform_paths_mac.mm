#include <Foundation/Foundation.h>

#include "platform_paths.hpp"

fdn::PathType fdn::getConfigurationPath()
{
    fdn::PathType path;
    @autoreleasepool {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
        NSString *applicationSupportDirectory = [paths firstObject];
        NSString *configurationPath = [NSString stringWithFormat:@"%@%@", applicationSupportDirectory, @"/CodecFoundation/"];
        path = [[configurationPath stringByExpandingTildeInPath] cStringUsingEncoding:NSUTF8StringEncoding];
        
    }
    return path;
}
