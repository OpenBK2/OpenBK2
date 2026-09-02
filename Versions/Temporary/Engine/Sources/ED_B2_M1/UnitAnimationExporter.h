#pragma once

#include "ED_Common/BasicExporter.h"

#include <cstdint>

class CInfantryExporter : public CBasicExporter
{
	OBJECT_NOCOPY_METHODS( CInfantryExporter );
	
	std::unordered_map<uint32_t, std::list<std::string> > animsMap;
	//
	void BuildAnimsMap();
	//
	bool ProcessInfantrySpecificAnimations( IManipulator *pItUnit );
	bool ProcessMechUnitLikeAnimations( IManipulator *pItUnit );
	//
	bool ProcessAABB( IManipulator *pMan );
	bool ProcessShootPoint( IManipulator *pMan );
public:
	// IExporter
	void FinishExport( const std::string &rszObjectTypeName, bool bForce );
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
	// check infantry unit
	EXPORT_RESULT CheckObject( IManipulator* pManipulator,
														 const std::string &rszObjectTypeName,
														 const std::string &rszObjectName,
														 bool bExport,
														 EXPORT_TYPE exportType );
};


