#include "StdAfx.h"

#include "ManuverInternal.h"
#include "PathFractionArcLine3D.h"
#include "PathFractionComposite.h"
#include "PathFractionArcLineArc.h"
#include "PathFractionArcLineArc3D.h"
#include "PlanesFormation.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "System/FastMath.h"
#include "System/RandomGen.h"
#include "Stats_b2_m1/DBPlaneManuvers.h"
#include "System/Commands.h"

extern float g = 0.0000983f;
extern NTimer::STime curTime;


//	SPlanesConsts


START_REGISTER(PlaneConsts)
REGISTER_VAR_EX( "temp.plane_min_height", NGlobal::VarFloatHandler, &SPlanesConsts::MIN_HEIGHT_TEMP, 0.0f, STORAGE_SAVE );
FINISH_REGISTER

float SPlanesConsts::MIN_HEIGHT = 300.0f;
float SPlanesConsts::MIN_HEIGHT_TEMP = 300.0f;
float SPlanesConsts::MAX_HEIGHT = 1500.0f;


//	CManuverBuilder ::




//	CManuver


void CManuver::GetManuverParams( struct SPrevPathParams *pParams ) const
{
	pParams->fDistToGo = fDistToGo;
	pParams->fCurTiltSpeed = fCurTiltSpeed;
	pPath->GetPrevPoints( pParams );
}	

const CVec3 CManuver::CalcPredictedPoint( CPlanesFormation *pPos, CPlanesFormation *pEnemy )
{
	// distance to enemy
	//const float fDist = fabs( pEnemy->GetPosB2() - pPos->GetPosB2() );
	// speed
	//const float fSpeed = fabs( pPos->GetSpeedB2() );
	// time
	//const float fTime = fDist / fSpeed;
	//return pEnemy->GetManuver()->GetProspectivePoint( fTime );
	return pEnemy->GetManuver()->GetProspectivePoint( 1000 );
}

const CVec3 CManuver::CalcPredictedSpeed( class CPlanesFormation *pPos, class CPlanesFormation *pEnemy )
{
	return pEnemy->GetManuver()->GetProspectiveSpeed( 1000 );
}

CVec3 CManuver::GetProspectiveSpeed( NTimer::STime nTime ) const
{
	// assume that speed is constant
	const float fAdd = nTime * fSpeed;
	const float fDiff = fAdd + fProgress  - pPath->GetLength();

	if ( fDiff > 0.0f ) // assume that further movement is by line
		return pPath->GetEndTangent();
	else
		return pPath->GetTangent( fAdd + fProgress );
}

CVec3 CManuver::GetProspectivePoint( const NTimer::STime nTime ) const
{
	// assume that speed is constant
	const float fAdd = nTime * fSpeed;
	const float fDiff = fAdd + fProgress  - pPath->GetLength();
	
	if ( fDiff > 0.0f ) // assume that further movement is by line
		return pPath->GetEndPoint() + pPath->GetEndTangent() * fSpeed * fDiff;
	else
		return pPath->GetPoint( fProgress + fAdd );
}

void CManuver::InitCommon( struct IPathFraction *_pPath, CPlanesFormation *_pPlane, const bool _bToHorisontal )
{
	InitInternal( _pPath, _pPlane, fabs( _pPlane->GetSpeedB2() ), _pPlane->GetNormalB2(), _bToHorisontal );
	CheckToHorisontal();
	fProgress = 0;
}

void CManuver::InitInternal( struct IPathFraction *_pPath, CPlanesFormation *_pPlane, const float _fSpeed, const CVec3 &_vNormale, const bool _bToHorisontal )
{
	bToHorisontal = _bToHorisontal;
	CPtr<IPathFraction> pTmp = _pPath; // to ensure deletion
	pPlane = _pPlane;
	// substitution
	pPath = new CPathFractionComposite( _pPlane, _pPath );
	SPrevPathParams param( _pPlane );

	fCurTiltSpeed = param.fCurTiltSpeed;
	fDistToGo = 0.0f;

	fSpeed = _fSpeed;
	fProgress = 0;
	
	CalcPoint();
	CalcSpeed();
	vNormal = _vNormale;
	CalcNormale( 0 );
	//NStr:://DebugTrace( NStr::Format( "Initted(%f,%f,%f)\n", vSpeed.x, vSpeed.y,vSpeed.z ) );
}

void CManuver::CalcSpeed()
{
	vSpeed = pPath->GetTangent();
	vSpeed *= fSpeed;
}

void CManuver::CalcPoint()
{
	vCenter = pPath->GetPoint();
}

void CManuver::AdjustNormale( const NTimer::STime timeDiff, const CVec3 &vDesiredNormale, CVec3 *vNormal, int *pnRotation, float *pfTiltToGo ) const
{
	// adjust normal according to speed direction change
	*vNormal -= vSpeed * ( *vNormal * vSpeed );
	Normalize( vNormal );

	// calculate normal
	// 	vNormal - former normal.
	// move to coordinate system (j = Normale, k = Speed ^ Normale)
	CVec3 k( vSpeed ^ vDesiredNormale );
	Normalize( &k );

	const CVec2 vDesiredNormaleT( 1, 0 );
	const CVec2 vCurrentNormaleT( vDesiredNormale * *vNormal, k * *vNormal );
	const WORD wCur( GetDirectionByVector( vCurrentNormaleT ) );
	const WORD wDesiredDir( GetDirectionByVector( vDesiredNormaleT ) );
	const float fDiff( DirsDifference( wDesiredDir, wCur ) );
	const int nDiffSign( DifferenceSign( wDesiredDir, wCur ) );

	if ( fDiff < fabs( timeDiff * fCurTiltSpeed ) )
	{
		// tilt direction match needed direction and it is near to finish
		*vNormal = vDesiredNormale;
		*pnRotation = 0;
		*pfTiltToGo = 0;
	}
	else if ( 0.0f == fCurTiltSpeed )
	{
		// start tilt 
		*pnRotation = nDiffSign;
		*pfTiltToGo = DirsDifference( wDesiredDir, wCur );
	}
	else
	{
		// continue tilt
		*pnRotation = DifferenceSign( wDesiredDir, wCur );
		const WORD wCalculatedDir( wCur + WORD(timeDiff * fCurTiltSpeed) );
		CVec2 vCalculatedNormale( GetVectorByDirection( wCalculatedDir ) );
		if ( timeDiff != 0 )
			*vNormal = vCalculatedNormale.x * vDesiredNormale + vCalculatedNormale.y * k;
		*pfTiltToGo = DirsDifference( wDesiredDir, wCalculatedDir );
	}
}

void CManuver::CalcNormale( const NTimer::STime timeDiff )
{
	CVec3 vDesiredNormale = pPath->GetNormale();
	Normalize( &vDesiredNormale );

	int nRotation = 0;										// if dirs difference is positive, then 1.
	float fTiltToGo = 0;									// additional tilt angle to reach desired tilt
	AdjustNormale( timeDiff, vDesiredNormale, &vNormal, &nRotation, &fTiltToGo );
	
	float fPositiveAccel = 0.0f;
	const float fTiltAccel = pPlane->GetPreferencesB2().GetTiltAccell();

	// if positive rotation and _don't need_ to decrease tilt speed, then TRUE
	// if negative rotation and _need_ decrease tilt speed, then TRUE
	const bool bNeedDecSpeed = fTiltToGo < 2.0f * fCurTiltSpeed * fCurTiltSpeed / fTiltAccel;

	if ( ( 1 == nRotation && !bNeedDecSpeed ) || ( -1 == nRotation && bNeedDecSpeed ) )
		fPositiveAccel = 1.0f;
	else if ( ( -1 == nRotation && !bNeedDecSpeed ) || ( 1 == nRotation && bNeedDecSpeed ) )
		fPositiveAccel = -1.0f;
  
	const float fTiltSpeedDelta = fPositiveAccel * fTiltAccel * timeDiff;
	if ( /*0.0f == fTiltToGo && */ 0 != fCurTiltSpeed && Sign( fCurTiltSpeed + fTiltSpeedDelta ) != Sign( fCurTiltSpeed ) )
		fCurTiltSpeed = 0;
	else
	{
		const float fMaxTiltSpeed = pPlane->GetPreferencesB2().GetTiltSpeed();
		fCurTiltSpeed += fTiltSpeedDelta;
		Clamp( fCurTiltSpeed, -fMaxTiltSpeed, fMaxTiltSpeed );
	}
}

const float CManuver::CalcHeightToEnterHorisontal( const CVec3 &_vSpeed, const CPlanePreferences &pref ) const
{
	const float fTurnRadius = pref.GetR( fabs(_vSpeed) );
	const float fAlpha = NMath::ASin( vSpeed.z / fabs( vSpeed ) );
	const float fSinAHalf = NMath::Sin( 0.5f * fAlpha );
	return 2.0f * fTurnRadius * sqr( fSinAHalf );
}

bool CManuver::IsChangeHeightPossible( const CVec3 &_vSpeed, const CVec3 &_vPos, const CPlanePreferences &pref, CVec3 *pvNewPoint ) const
{
	const float fCrit = CalcHeightToEnterHorisontal( _vSpeed, pref );

	const SVector tile( AICellsTiles::GetTile( _vPos.x, _vPos.y ) );
	const float fHeight = GetHeights()->GetTileHeight( tile.x, tile.y );
	const CVec3 vPos( _vPos.x, _vPos.y, _vPos.z - fHeight );

	if ( !bToHorisontal &&
			 (_vSpeed.z < 0 && fCrit > vPos.z - SPlanesConsts::GetMinHeight() ) ||
			 (_vSpeed.z > 0 && fCrit > SPlanesConsts::MAX_HEIGHT - vPos.z ) )
	{
		CVec2 vHorDirection( _vSpeed.x, _vSpeed.y );
		if ( vHorDirection == VNULL2 )
			vHorDirection = GetVectorByDirection( NRandom::Random( 65535 ) );
		Normalize( &vHorDirection );

		// calculate offset to exit from dive
		const CVec2 vHorisontalOffset( 2.1f * vHorDirection * pref.GetR( pref.GetMaxSpeed() ) );
		*pvNewPoint = CVec3 (vPos.x, vPos.y, 0.0f) + CVec3( vHorisontalOffset, fHeight + pref.GetPatrolHeight() );
		return false;
	}
	return true;
}


bool CManuver::AdvanceCommon( const NTimer::STime timeDiff )
{
	const float fDist = fSpeed * timeDiff;
	const bool bPathFinished = pPath->Iterate( fDist, &fDistToGo );
	const CVec3 vFormerCenter( vCenter );
	CalcPoint();
	fProgress += fabs( vFormerCenter - vCenter );
	float fDz = vCenter.z;

	fDz -= vCenter.z;

	const CPlanePreferences &pref = pPlane->GetPreferencesB2();
	fSpeed = pref.GetSpeed( vCenter.z );

	CalcSpeed();
	CVec3 vTmp = vNormal; 
	CalcNormale( timeDiff );
	if ( vNormal == VNULL3 )
		vNormal = vTmp;

	CheckToHorisontal();

	return bPathFinished;
}

void CManuver::CheckToHorisontal()
{
	if ( bToHorisontal ) // already going to horisontal
		return;

	CVec3 vNewTarget;
	const CPlanePreferences &pref = pPlane->GetPreferencesB2();
	if ( !pref.CanViolateHeghtLimits() && vSpeed.z != 0.0f && !IsChangeHeightPossible( vSpeed, vCenter, pref, &vNewTarget ) )
	{
		//DEBUG{
		static int nRedirect = 0;
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 4, 
			StrFmt( "Change Height impossible, redirect #%i (%.2f,%.2f,%.2f)", nRedirect, vNewTarget.x, vNewTarget.y, vNewTarget.z ) );
		++nRedirect;
		//DEBUG}

		CPathFractionArcLine3D * pNewPath = new CPathFractionArcLine3D;
		SPrevPathParams param( pPlane );
		pNewPath->Init( param, vNewTarget, pPlane->GetPreferencesB2().GetR( fSpeed ) );
		InitCommon( pNewPath, pPlane, true );
	}
}

CVec3 CManuver::GetNormale() const 
{ 
	return vNormal + 2.0f * V3_AXIS_Z * !pPlane->GetPreferencesB2().CanFlip(); 
}


//	CManuverSteepClimb


void CManuverSteepClimb::Init( const NDb::EManuverDestination dest, CPlanesFormation *_pPlane, CPlanesFormation *pEnemy )
{
	NI_ASSERT( EMD_MANUVER_DEPENDENT == dest, "CANNOT DO GORKA ANYWERE OTHER THEN EMD_MANUVER_DEPENDENT" );
	//CRAP{ SOME DIFFERENCES BASED ON DISTANCE WILL BE GOOD
	Init( _pPlane );
	//CRAP}
}

void CManuverSteepClimb::Init( CPlanesFormation *pPos )
{
	const CPlanePreferences &pref = pPos->GetPreferencesB2();

	CPathFractionArcLine3D *pNewPath = new CPathFractionArcLine3D ;
	
	const CVec3 vPos( pPos->GetPosB2() );
	CVec3 vSpeed ( pPos->GetSpeedB2() );
	CVec2 vHorSpeed( vSpeed.x, vSpeed.y );

	float fPathLength = pref.GetR( fabs(vSpeed) ) * 2.0f; 
	Normalize( &vHorSpeed );
	const CVec3 vDesiredPos( vPos + CVec3( fPathLength * vHorSpeed, fPathLength / 2.0f ) );
	
	SPrevPathParams param( pPos );
	pNewPath->Init( param, vDesiredPos, pref.GetR( fabs(vSpeed) ) );
	
	InitCommon( pNewPath, pPos, false );
}

bool CManuverSteepClimb::Advance( const NTimer::STime timeDiff )
{
	// determine if it is circle path fraction or not.
	const bool bRet = AdvanceCommon( timeDiff );
	return bRet;
}


//	CManuverToHorisontal


void CManuverToHorisontal::Init( CPlanesFormation *pPos, const CVec3 &vPos )
{
	const CPlanePreferences &pref = pPos->GetPreferencesB2();
	// create manuver to enter horisontal 
	const CVec3 vCurrentPos( pPos->GetPosB2() );
	const CVec3 vCurrentSpeed( pPos->GetSpeedB2() );
	
	CVec3 vDesiredDir( vPos - vCurrentPos );
	Normalize( &vDesiredDir );

	const float fR ( pref.GetR( fabs(pPos->GetSpeedB2()) ) );
	SPrevPathParams param( pPos );

	if ( vCurrentSpeed.z < 0.00001f && fabs( vPos.z - vCurrentPos.z ) < 0.00001f ) 
	{
		// already at desired height
		CPtr<CPahtFractionArcLineArc> pNewPath = new CPahtFractionArcLineArc;
		pNewPath->Init( param, vPos, VNULL3, fR, 0.0f );
		InitCommon( pNewPath, pPos, false );
	}
	else
	{
		CPtr<CPahtFractionArcLineArc3D> pNewPath = new CPahtFractionArcLineArc3D;
		pNewPath->Init( param, vPos, vDesiredDir, fR, fR );
		InitCommon( pNewPath, pPos, false );
	}
}

bool CManuverToHorisontal::Advance( const NTimer::STime timeDiff )
{
	const bool bRet = AdvanceCommon( timeDiff );
	return bRet;
}


//	CManuverPrepareGroundAttack::


void CManuverPrepareGroundAttack::Init( CPlanesFormation *pPos, const CVec3 &vPos, const CVec3 &vDirection )
{
	const CPlanePreferences &pref = pPos->GetPreferencesB2();
	const CVec3 vCurrentPos( pPos->GetPosB2() );
	const CVec3 vCurrentSpeed( pPos->GetSpeedB2() );

	CVec3 vDesiredDir( vDirection );
	Normalize( &vDesiredDir );

	const float fR ( pref.GetR( fabs(pPos->GetSpeedB2()) ) );
	SPrevPathParams param( pPos );

	CPtr<CPahtFractionArcLineArc3D> pNewPath = new CPahtFractionArcLineArc3D;
	pNewPath->Init( param, vPos, vDesiredDir, fR, fR );
	InitCommon( pNewPath, pPos, false );
}

bool CManuverPrepareGroundAttack::Advance( const NTimer::STime timeDiff )
{
	const bool bRet = AdvanceCommon( timeDiff );
	return bRet;
}


//	CManuverGeneric::


void CManuverGeneric::Init( CPlanesFormation *pPos, const CVec3 &vPos )
{
	const CPlanePreferences &pref = pPos->GetPreferencesB2();
	CPtr<CPathFractionArcLine3D> pNewPath = new CPathFractionArcLine3D();
	SPrevPathParams param( pPos );
	pNewPath->Init( param, vPos, pref.GetR( fabs(pPos->GetSpeedB2()) ) );
	InitCommon( pNewPath, pPos, false );
}

bool CManuverGeneric::Advance( const NTimer::STime timeDiff )
{
	const bool bRet = AdvanceCommon( timeDiff );
	return bRet;
}

BASIC_REGISTER_CLASS( IManuver );
REGISTER_SAVELOAD_CLASS( 0x1108EB01, CManuverSteepClimb );
REGISTER_SAVELOAD_CLASS( 0x1108EB02, CManuverGeneric );
REGISTER_SAVELOAD_CLASS( 0x1109B380, CManuverToHorisontal );
REGISTER_SAVELOAD_CLASS( 0x111BD3C0, CManuverPrepareGroundAttack )

