#pragma once

#include "MapEditorLib/Interface_UserData.h"

class CUserDataContainer : public IUserDataContainer
{
	OBJECT_NOCOPY_METHODS( CUserDataContainer );
	SUserData userData;

public:
	CUserDataContainer();
	~CUserDataContainer() override;

	void GetXMLFilePath( std::string *pszXMLFilePath );
	void GetConstXMLFilePath( std::string *pszConstXMLFilePath );

	// IUserDataContainer
	SUserData *Get() override
	{
		return &userData;
	}

	//
	void Load() override;
	void Save() override;
};
