#include "stdafx.h"

#include "DefaultsOptions.h"
#include "Interface_UserData.h"

namespace NEditorDefaults
{
SOptions LoadOptions()
{
	SOptions options;
	Singleton<IUserDataContainer>()->Get()->SerializeSettings(
			options, "Defaults", SUserData::PRIVATE_SETTINGS, SUserData::ST_LOAD );
	// Missing or empty preferences keep new reinforcement points usable out of the box.
	return options.szDeployTemplate.empty() ? SOptions() : options;
}

bool SaveOptions( SOptions options )
{
	if ( options.szDeployTemplate.empty() )
	{
		return false;
	}
	return Singleton<IUserDataContainer>()->Get()->SerializeSettings(
			options, "Defaults", SUserData::PRIVATE_SETTINGS, SUserData::ST_SAVE );
}
} // namespace NEditorDefaults
