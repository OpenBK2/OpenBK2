#pragma once

#include "../MapEditorLib/Interface_UserData.h"

class CUserDataContainer : public IUserDataContainer
{
	OBJECT_NOCOPY_METHODS( CUserDataContainer );
	SUserData userData;

public:
	// IUserDataContainer
	SUserData* Get() { return &userData; }
	void Load() {}
	void Save() {}
};



