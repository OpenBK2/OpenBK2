#if !defined(__UNIT_ANIMATION_EXPORTER__)
#define __UNIT_ANIMATION_EXPORTER__
#pragma once

#include "../ED_Common/BasicExporter.h"

class CInfantryExporter : public CBasicExporter
{
	OBJECT_NOCOPY_METHODS( CInfantryExporter );
	
	hash_map<DWORD, list<string> > animsMap;
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
	void FinishExport( const string &rszObjectTypeName, bool bForce );
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
	// check infantry unit
	EXPORT_RESULT CheckObject( IManipulator* pManipulator,
														 const string &rszObjectTypeName,
														 const string &rszObjectName,
														 bool bExport,
														 EXPORT_TYPE exportType );
};

#endif // !defined(__UNIT_ANIMATION_EXPORTER__)
