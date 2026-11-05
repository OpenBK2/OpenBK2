#pragma once

#include "Stats_B2_M1/DBMapInfo.h"

#include "AILogic_export.h"

namespace NDb
{
struct SMapInfo;
}

class CKeyBuildingBonusSystem
{
	typedef det_map<int /*nLinkID*/, NDb::SPlayerBonusData> CBuildingBonuses;
	ZDATA
	CBuildingBonuses buildingBonuses;

public:
	ZEND int operator&( IBinSaver &f )
	{
		f.Add( 2, &buildingBonuses );
		return 0;
	}

public:
	// all buildings will notify about ownership change
	AILOGIC_EXPORT void ChangeOwnership( int nOldPlayer, int nNewPlayer, int nLinkID, bool bDuringMapLoad );
	bool IsStorage( int nLinkID ) const;
	AILOGIC_EXPORT void InitBonusSystem( const NDb::SMapInfo *pMapInfo );
	bool IsKeyBuilding( int nLinkID ) const;

	void SendUpdates() const;

	void Clear()
	{
		buildingBonuses.clear();
	}
};
