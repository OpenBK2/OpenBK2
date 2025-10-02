#pragma once

#include "MapEditorLib/Interface_MOD.h"

class CMODContainer : public IMODContainer
{
	OBJECT_NOCOPY_METHODS( CMODContainer );

public:
	CMODContainer() {}
	~CMODContainer() {}

	// IMODContainer
	bool CanNewMOD();
	bool CanOpenMOD();
	bool CanCloseMOD();
	//
	bool NewMOD();
	bool OpenMOD();
	void CloseMOD();
	//
	/**
	bool IsValidFolder( const string &rszFolder );
	bool IsValidPath( const string &rszPath );
	/**/
	string GetDataFolder( SUserData::ENormalizePathType eNormalizePathType );
};



