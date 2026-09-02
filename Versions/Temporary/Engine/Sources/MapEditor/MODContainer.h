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
	bool IsValidFolder( const std::string &rszFolder );
	bool IsValidPath( const std::string &rszPath );
	/**/
	std::string GetDataFolder( SUserData::ENormalizePathType eNormalizePathType );
};



