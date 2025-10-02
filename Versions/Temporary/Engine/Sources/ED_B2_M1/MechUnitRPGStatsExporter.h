#if !defined(__MECHUNITRPGSTATS_EXPORTER__)
#define __MECHUNITRPGSTATS_EXPORTER__
#pragma once

#include "HPObjectRPGStatsExporter.h"

class CMechUnitRPGStatsExporter : public CHPObjectRPGStatsExporter
{
	OBJECT_NOCOPY_METHODS( CMechUnitRPGStatsExporter );
	//
	bool ProcessMechUnitAnimations( IManipulator *pItUnit );
	void CopyAnimationsToObject( IManipulator *pUnitManipulator, IManipulator *pAttachedManipulator );
	void CopyAnimationsToAttached( IManipulator *pUnitManipulator );
	void ProcessAttachedObjects( IManipulator *pUnitManipulator );

	bool ProcessAABB( IManipulator *pItUnit );
	//
	CMechUnitRPGStatsExporter() {}
public:
	// IExporter
	EXPORT_RESULT ExportObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bForce,
															EXPORT_TYPE exportType );
	//
	EXPORT_RESULT CheckObject( IManipulator* pManipulator,
															const string &rszObjectTypeName,
															const string &rszObjectName,
															bool bExport,
															EXPORT_TYPE exportType );
};

#endif// __MECHUNITRPGSTATS_EXPORTER__


