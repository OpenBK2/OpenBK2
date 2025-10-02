#include "stdafx.h"

#include "PresizePath.h"
#include "TankpitPath.h"
#include "Technics.h"
#include "..\Common_RTS_AI\StandartSmoothMechPath.h"

REGISTER_SAVELOAD_CLASS( 0x1108D4C4, CPresizePath );
REGISTER_SAVELOAD_CLASS( 0x11123400, CMechUnitRestOnBoardPath);

//*******************************************************************
//*												CPresizePath															*
//*******************************************************************

CPresizePath::CPresizePath( CBasePathUnit *_pUnit, const class CVec2 &vEndPoint, const class CVec2 &vEndDir )
:	vEndPoint( vEndPoint ), vEndDir( vEndDir ), eState( EPPS_APPROACH_BY_STANDART ), fSpeedLen( 0.0f ),
	pUnit( _pUnit )
{
	wDesiredDir = GetDirectionByVector(vEndDir);

	pPathStandart = new CStandartSmoothPath();
/*

	if ( pUnit->GetTurnSpeed() == 0.0f )
		pPathStandart = new CStandartSmoothPath();
	else
		pPathStandart = new CStandartSmoothMechPath();

	pPathStandart->SetOwner( pUnit );
*/
}

bool CPresizePath::Init( CBasePathUnit *_pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap )
{
	pUnit =_pUnit;
	CPtr<IPath> p = pPath;
	pPathStandart->Init( pUnit, pPath, bSmoothTurn, bCheckTurn, GetAIMap() );
	eState = EPPS_APPROACH_BY_STANDART;

	return true;
}

bool CPresizePath::InitByFormationPath( CFormation *pFormation, CBasePathUnit *_pUnit )
{
	pUnit = _pUnit;
	if ( eState == EPPS_APPROACH_BY_STANDART)
		pPathStandart->InitByFormationPath( pFormation, pUnit );

	return true;
}

bool CPresizePath::Init( IMemento *pMemento, CBasePathUnit *_pUnit, CAIMap *pAIMap )
{
	CPtr<IMemento> p = pMemento;
	pUnit = _pUnit;
	if ( eState == EPPS_APPROACH_BY_STANDART && pMemento )
		pPathStandart->Init( pMemento, pUnit, GetAIMap() );

	return true;
}

bool CPresizePath::IsFinished() const
{
	return EPPS_FINISHED == eState;
}

void CPresizePath::Segment( const NTimer::STime timeDiff )
{
	// CRAP{ unknown bug
	//if ( pUnit && !pPathStandart->GetOwner() )
	//	pPathStandart->SetOwner( pUnit );
	// CRAP}

	switch ( eState )
	{
		case EPPS_WAIT_FOR_INIT:
			break;
		case EPPS_APPROACH_BY_STANDART:
			if ( !pPathStandart->IsFinished() )
			{
				pPathStandart->Segment( timeDiff );
				return;
			}
			else
				eState = EPPS_TURN_TO_DESIRED_POINT;

			break;
		case EPPS_TURN_TO_DESIRED_POINT:
			{
				WORD dir = GetDirectionByVector( vEndPoint - pUnit->GetCenterPlain() );
				dir = !pUnit->IsGoForward() ? dir : dir+65535/2;
				if ( pUnit->TurnToDirection( dir, true, true ) )
				{
					pPathCheat = new CTankPitPath( pUnit, pUnit->GetCenterPlain(), vEndPoint );
					eState = EPPS_APPROACH_DESIRED_POINT;
				}
			}
			break;
		case EPPS_APPROACH_DESIRED_POINT:
			if ( !pPathCheat->IsFinished() )
			{
				pPathCheat->Segment( timeDiff );
				return;
			}
			else
				eState = EPPS_TURN_TO_DESIRED_DIR;

			break;
		case EPPS_TURN_TO_DESIRED_DIR:
			if ( pUnit->TurnToDirection( wDesiredDir, true, true ) )
				eState = EPPS_FINISHED;

			break;
	}
//	return CVec3( pUnit->GetCenter(), 0 );
}

void CPresizePath::Stop()
{
	if ( eState == EPPS_APPROACH_BY_STANDART)
		pPathStandart->Stop();
}

/*
float& CPresizePath::GetSpeedLen()
{ 
	if ( eState == EPPS_APPROACH_BY_STANDART)
		return pPathStandart->GetSpeedLen(); 
	if ( eState == EPPS_APPROACH_DESIRED_POINT)
		return pPathCheat->GetSpeedLen();
	return fSpeedLen;
}
*/

/*
void CPresizePath::NotifyAboutClosestThreat( CBasePathUnit *pCollUnit, const float fDist ) 
{
	if ( eState == EPPS_APPROACH_BY_STANDART)
		pPathStandart->NotifyAboutClosestThreat( pCollUnit, fDist ) ;
}
*/

/*
void CPresizePath::SlowDown()
{
	if ( eState == EPPS_APPROACH_BY_STANDART )
		pPathStandart->SlowDown();
}
*/

const bool CPresizePath::CanGoBackward() const 
{ 
	if ( eState == EPPS_APPROACH_BY_STANDART)
		return pPathStandart->CanGoBackward();
	return false;
}

const bool CPresizePath::CanGoForward() const
{
	if ( eState == EPPS_APPROACH_BY_STANDART)
		return pPathStandart->CanGoForward();
	return false;
}

/*
void CPresizePath::GetNextTiles( list<SVector> *pTiles ) 
{
	if ( eState == EPPS_APPROACH_BY_STANDART)
		pPathStandart->GetNextTiles( pTiles ) ;
}
*/

const CVec2 CPresizePath::PeekPathPoint( const int nToShift ) const 
{ 
	if ( eState == EPPS_APPROACH_BY_STANDART)
		return pPathStandart->PeekPathPoint( nToShift );
	return VNULL2;
}

IMemento* CPresizePath::CreateMemento() const 
{
	if ( eState == EPPS_APPROACH_BY_STANDART)
		return pPathStandart->CreateMemento();
	return 0;
}

/*
void CPresizePath::SetOwner( CBasePathUnit *_pUnit )
{ 
	pUnit = _pUnit;
	
	if ( IsValid( pPathStandart ) )
		pPathStandart->SetOwner( pUnit );
	if ( IsValid( pPathCheat ) )
		pPathCheat->SetOwner( pUnit );
}
*/

//*******************************************************************
//*												CMechUnitRestOnBoardPath															*
//*******************************************************************

CMechUnitRestOnBoardPath::CMechUnitRestOnBoardPath( CBasePathUnit *_pUnit, CMilitaryCar *_pTransport )
: pUnit( _pUnit ), pTransport( _pTransport ), fSpeedLen( 0.0f )
{
	vFormerPlacement = vCurrentPlacement = pTransport->GetBoardedPosition( checked_cast<CAIUnit*>( pUnit ), 0 );
}

void CMechUnitRestOnBoardPath::Segment( const NTimer::STime timeDiff )
{
	if ( pUnit )
	{
		pUnit->SetDirection( pTransport->GetBoardedDirection( checked_cast<CAIUnit*>( pUnit ), 0 ) );
		vFormerPlacement = vCurrentPlacement;
		vCurrentPlacement = pTransport->GetBoardedPosition( checked_cast<CAIUnit*>( pUnit ), 0 );
		vCurrentPlacement.z += 1;
		pUnit->SetCenter( vCurrentPlacement, true );			// Set unit to where it belongs
	}
}

void CMechUnitRestOnBoardPath::Advance()
{
	//CRAP{ UNTILL 
	if ( pUnit )
	{
		pUnit->SetDirection( pTransport->GetBoardedDirection( checked_cast<CAIUnit*>( pUnit ), 0 ) );
		vFormerPlacement = vCurrentPlacement;
		vCurrentPlacement = pTransport->GetBoardedPosition( checked_cast<CAIUnit*>( pUnit ), 0 );
		vCurrentPlacement.z += 1;
		pUnit->SetCenter( vCurrentPlacement, true );			// Set unit to where it belongs
	}
	//CRAP}
}

const CVec3 CMechUnitRestOnBoardPath::GetPoint( NTimer::STime timeDiff )
{
	return vCurrentPlacement;
}

void CMechUnitRestOnBoardPath::GetSpeed3( CVec3 *vSpeed ) const
{
	*vSpeed = ( vCurrentPlacement - vFormerPlacement ) / SConsts::AI_SEGMENT_DURATION;
}

void CMechUnitRestOnBoardPath::OnSerialize( IBinSaver &saver )
{
  SerializeBasePathUnit( saver, 127, &pUnit );
}

