#pragma once

#include "Stats_B2_M1/DBPassProfile.h"

#include <cstdint>

struct SUnitProfile;

class CObjectProfile : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CObjectProfile );

	ZDATA
		NDb::SPassProfile profile;
		CVec2 vCenter;
		CVec2 vRotation;
		CTRect<float> aabbRect;
		det_set<SVector, STilesHash> tilesUnder;	// for check
		std::vector<SVector> tilesUnderVector;	// for get tiles
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&profile); f.Add(3,&vCenter); f.Add(4,&vRotation); f.Add(5,&aabbRect); f.Add(6,&tilesUnder); f.Add(7,&tilesUnderVector); return 0; }

	void AddCentersOfSmallPolygons();
	void Init( const NDb::SPassProfile &profile, const CVec2 &vCenter, const CVec2 &vRotation, const bool bForceThickLock );
public:
	CObjectProfile() { }
	CObjectProfile( const NDb::SPassProfile &profile, const CVec3 &vCenter3D, const uint16_t wDir, const bool bForceThickLock );
	CObjectProfile( const struct SRect &unitRect, const bool bForceThickLock );

	bool IsTileInside( const SVector &tile ) const;
	bool IsPointInside( const CVec2 &vPoint ) const;
	bool IsWeakIntersected( const struct SRect &unitRect ) const;

	const std::vector<SVector>& GetTilesUnder() const { return tilesUnderVector; }
	const CTRect<float>& GetAABBRect() const { return aabbRect; }
};


