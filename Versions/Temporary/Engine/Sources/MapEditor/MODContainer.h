#pragma once

#include "MapEditorLib/Interface_MOD.h"

class CMODContainer : public IMODContainer
{
	OBJECT_NOCOPY_METHODS( CMODContainer );

public:
	CMODContainer() {}

	~CMODContainer() override {}

	// IMODContainer
	bool CanNewMOD() override;
	bool CanOpenMOD() override;
	bool CanCloseMOD() override;
	//
	bool NewMOD() override;
	bool OpenMOD() override;
	void CloseMOD() override;
	//
	/*
	bool IsValidFolder( const std::string &rszFolder );
	bool IsValidPath( const std::string &rszPath );
	*/
	std::string GetDataFolder( SUserData::ENormalizePathType eNormalizePathType ) override;
};
