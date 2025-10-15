#include "stdafx.h"

#include "UpdatableObject.h"
#include "Diplomacy.h"
#include "Cheats.h"
#include "Common_RTS_AI/StaticMapHeights.h"

#include "AILogic_export.h"

extern CDiplomacy theDipl;
extern SCheats theCheats;

BASIC_REGISTER_CLASS( AILOGIC, CUpdatableObj );

float CUpdatableObj::GetTerrainHeight( const float x, const float y, const NTimer::STime timeDiff ) const
{
	return GetHeights()->GetVisZ( x, y );
}

const bool CUpdatableObj::IsVisibleByPlayer()
{
	return IsVisible( theDipl.GetMyParty() );
}


