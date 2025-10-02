#if !defined(__FENCE_RPG_STATS_EXPORTER__)
#define __FENCE_RPG_STATS_EXPORTER__

#pragma once

#include "StaticObjectRPGStatsExporter.h"

class CFenceRPGStatsExporter : public CStaticObjectRPGStatsExporter
{
	OBJECT_NOCOPY_METHODS( CFenceRPGStatsExporter );
	
	CFenceRPGStatsExporter() {}
	bool ExportVisobjs( IManipulator *pManipulator, 
											const string &rszSegmentsSetName, 
											const CArray2D<BYTE> &rPassabilityArray, 
											const CVec3 &rvPassabilityOrigin );

	void CreatePassProfiles( IManipulator *pManipulator, const string &rszSegmentsSetName );
	bool GetGeom0FileName( IManipulator *pManipulator, 
												const string &rszSegmentsSetName, 
												string *pszGeomFileName );
public:
	// IExporter
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
	//
};

#endif // !defined(__FENCE_RPG_STATS_EXPORTER__)

