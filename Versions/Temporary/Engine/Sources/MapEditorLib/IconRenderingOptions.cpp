#include "stdafx.h"

#include "IconRenderingOptions.h"
#include "Interface_UserData.h"

namespace NIconRendering
{
SOptions LoadOptions()
{
	SOptions options;
	// Private editor settings already resolve to the user's editor directory.
	Singleton<IUserDataContainer>()->Get()->SerializeSettings(
			options, "IconRendering", SUserData::PRIVATE_SETTINGS, SUserData::ST_LOAD );
	return options.IsValid() ? options : SOptions();
}

bool SaveOptions( SOptions options )
{
	if ( !options.IsValid() )
	{
		return false;
	}
	return Singleton<IUserDataContainer>()->Get()->SerializeSettings(
			options, "IconRendering", SUserData::PRIVATE_SETTINGS, SUserData::ST_SAVE );
}
} // namespace NIconRendering
