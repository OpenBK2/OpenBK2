#pragma once

#include "Interface_UserData.h"

struct IMODContainer : public CObjectBase
{
	enum { tidTypeID = 0x1424DC42 };

	virtual bool CanNewMOD() = 0;
	virtual bool CanOpenMOD() = 0;
	virtual bool CanCloseMOD() = 0;
	//
	virtual bool NewMOD() = 0;
	virtual bool OpenMOD() = 0;
	virtual void CloseMOD() = 0;
	//
	/**
	virtual bool IsValidFolder( const std::string &rszFolder ) = 0;
	virtual bool IsValidPath( const std::string &rszPath ) = 0;
	/**/
	virtual std::string GetDataFolder( SUserData::ENormalizePathType eNormalizePathType ) = 0;
};



