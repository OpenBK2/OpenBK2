#pragma once

#include "Stats_B2_M1/RPGStats.h"

#include <cstdint>

namespace NDb
{
	struct SReinforcementPosition;
	struct SReinforcement;
	struct SReinforcementEntry;
	enum EReinforcementType;
}

namespace NAI4Globe
{
	struct SGlobeObj;
}

namespace NReinforcement
{
struct SReinforcementTypeHash
{
	int operator()( const NDb::EReinforcementType &eType ) const
	{
		return static_cast<int>( eType );
	}
};
// new method
void PlaceSingleLandReinforcement( const int nPlayer, const NDb::SReinforcement *_pReinf, const EReinforcementType eType, const NDb::SDeployTemplate *pTemplate,
	const CVec2 &vPosition, uint16_t wDirection, const int nScriptID, std::list< std::pair<int, CObjectBase*> > *pObjects, const bool bDisableUpdates );

void PlaceSingleSeaReinforcement( const int nPlayer, const NDb::SReinforcement *_pReinf, const NDb::SDeployTemplate *pTemplate,
	const CVec2 &vPosition, uint16_t wDirection, const int nScriptID, const CVec2 &vTarget );
}


