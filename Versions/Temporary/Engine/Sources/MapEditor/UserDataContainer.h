#pragma once

#include "../MapEditorLib/Interface_UserData.h"

class CUserDataContainer : public IUserDataContainer
{
	OBJECT_NOCOPY_METHODS( CUserDataContainer );
	SUserData userData;

public:
	CUserDataContainer();
	~CUserDataContainer();

	void GetXMLFilePath( string *pszXMLFilePath );
	void GetConstXMLFilePath( string *pszConstXMLFilePath );

	// IUserDataContainer
	SUserData* Get() { return &userData; }
	//
	void Load();
	void Save();
};



