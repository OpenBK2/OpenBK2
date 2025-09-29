#include "stdafx.h"

#include "AIUnitInfoForGeneral.h"
#include "AIUnit.h"
#include "Diplomacy.h"
#include "General.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CSupremeBeing theSupremeBeing;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*											CAIUnitInfoForGeneral												*
//*******************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//BASIC_REGISTER_CLASS( CAIUnitInfoForGeneral );
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAIUnitInfoForGeneral::CAIUnitInfoForGeneral( CAIUnit *_pOwner )
: pOwner( _pOwner ),
	lastVisibleTime( 0 ), vLastVisiblePosition( VNULL2 ),
	lastVisibleAntiArtTime( 0 ), vLastVisibleAntiArtCenter( VNULL2 ), fDistToLastVisibleAntiArt( -1.0f ),
	fWeight( 0 ), vLastRegisteredGeneralPos( VNULL2 )
{
	nextTimeToReportGeneral = curTime + NRandom::Random( 2000, 5000 );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAIUnitInfoForGeneral::Segment()
{
	// если это юнит не управляемый AI и пришло время рассказать генералу о своём состоянии
	if ( curTime >= nextTimeToReportGeneral && theDipl.GetNeutralParty() != pOwner->GetParty() )
	{
		nextTimeToReportGeneral = curTime + NRandom::Random( 2000, 5000 );

		const int nEnemyParty = 1 - pOwner->GetParty();
		if ( pOwner->IsVisible( nEnemyParty ) )
		{
			lastVisibleTime = curTime;
			vLastVisiblePosition = pOwner->GetCenterPlain();
		}

		if ( lastVisibleTime != 0 || lastVisibleAntiArtTime != 0 )
		{
			//DebugTrace( "lastVisibleTime = %i, lastVisibleAntiArtTime = %i", lastVisibleTime, lastVisibleAntiArtTime );
			theSupremeBeing.UpdateEnemyUnitInfo(
				this,
				curTime - lastVisibleTime, vLastVisiblePosition,
				curTime - lastVisibleAntiArtTime, vLastVisibleAntiArtCenter, fDistToLastVisibleAntiArt 
			);
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAIUnitInfoForGeneral::UpdateVisibility( bool _bVisible )
{
	lastVisibleTime = curTime;
	vLastVisiblePosition = pOwner->GetCenterPlain();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAIUnitInfoForGeneral::UpdateAntiArtFire( const NTimer::STime lastHeardTime, const CVec2 &vAntiArtCenter )
{
	if ( curTime - lastHeardTime <= 1000 && lastHeardTime != 0 )
	{
		lastVisibleAntiArtTime = curTime;
		vLastVisibleAntiArtCenter = vAntiArtCenter;
		fDistToLastVisibleAntiArt = fabs( vAntiArtCenter - pOwner->GetCenterPlain() );
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAIUnitInfoForGeneral::Die()
{
	theSupremeBeing.UnitDied( this );
}
