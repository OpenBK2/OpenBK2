
#pragma once

#include "StaticObjectRPGStatsExporter.h"

#include <cstdint>

class CFenceRPGStatsExporter : public CStaticObjectRPGStatsExporter
{
	OBJECT_NOCOPY_METHODS( CFenceRPGStatsExporter );
	
	CFenceRPGStatsExporter() {}
	bool ExportVisobjs( IManipulator *pManipulator, 
											const std::string &rszSegmentsSetName, 
											const CArray2D<uint8_t> &rPassabilityArray,
											const CVec3 &rvPassabilityOrigin );

	void CreatePassProfiles( IManipulator *pManipulator, const std::string &rszSegmentsSetName );
	bool GetGeom0FileName( IManipulator *pManipulator, 
												const std::string &rszSegmentsSetName, 
												std::string *pszGeomFileName );
public:
	// IExporter
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const std::string &rszObjectTypeName,
															const std::string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
	//
};


