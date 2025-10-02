#pragma once

#include "..\Stats_B2_M1\DBMapInfo.h"

namespace NDb
{
	struct SMapInfo;
}

class CKeyBuildingBonusSystem
{
	typedef hash_map<int/*nLinkID*/, NDb::SPlayerBonusData> CBuildingBonuses;
	ZDATA
	CBuildingBonuses buildingBonuses;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&buildingBonuses); return 0; }
public:
	
	// all buildings will notify about ownership change
	void ChangeOwnership( int nOldPlayer, int nNewPlayer, int nLinkID, bool bDuringMapLoad );
	bool IsStorage( int nLinkID ) const;
	void InitBonusSystem( const NDb::SMapInfo * pMapInfo );
	bool IsKeyBuilding( int nLinkID ) const;

	void SendUpdates() const;
	
	void Clear() 
	{
		buildingBonuses.clear();
	}
};


