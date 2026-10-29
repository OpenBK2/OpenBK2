#include "PlatformPaths.h"

#include <stdexcept>

#import <Foundation/Foundation.h>

namespace NSettings
{

std::string GetMacApplicationSupportDirectory()
{
	@autoreleasepool
	{
		NSURL *url = [[[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory
																												 inDomains:NSUserDomainMask] firstObject];
		if ( !url )
		{
			throw std::runtime_error( "Cannot locate the user's Application Support directory." );
		}
		return std::string( [[url path] UTF8String] ) + "/com.OpenBK2Team.OpenBK2";
	}
}

} // namespace NSettings
