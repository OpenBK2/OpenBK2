#include "StdAfx.h"
#include "sound.h"
#include "SubstSound.h"
#include "SoundSceneConsts.h"
#include "SoundSceneInternal.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_SAVELOAD_CLASS( 0x110793C1, CSound );
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*															CSound*
//*******************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CSound::CSound(	int wID, 
							 const NDb::SSoundDesc *_pDesc,
														interface ISound *pSound, 
														const enum ESoundMixType eMixType,
														const CVec3 &vPos,
														const bool bLooped,
														const NDb::ESoundType eCombatType,
														float fMinRadius,
														float fMaxRadius )
: pSample( pSound ), 
	eMixType( eMixType ), 
	bLooped( bLooped ),
	wID( wID ),
	timeBegin( 0 ),
	timeBeginDim( 0 ) ,
	bStartedMark( false ),
	bFinishedMark( false ),
	bDimMark( false ),
	vPos( vPos ), vSpeed( VNULL3 ),
	pDesc( _pDesc ),
	nMinRadius( fMinRadius / SSoundSceneConsts::SS_SOUND_CELL_SIZE / SSoundSceneConsts::SS_TILE_SIZE  ),
	nMaxRadius( fMaxRadius / SSoundSceneConsts::SS_SOUND_CELL_SIZE / SSoundSceneConsts::SS_TILE_SIZE ),
	eCombatType( eCombatType ), timeLastPosUpdate( CSoundScene2D::GetCurTime() )
{
	nMaxRadius = nMaxRadius == 0 ? 1 : nMaxRadius;
	nMinRadius = nMinRadius == 0 ? 1 : nMinRadius;
	timeToPlay = pSample == 0 ? 0 : 1000 * pSample->GetLenght() / pSample->GetSampleRate();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSound::~CSound()
{
	int a = 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSound::IsTimeToFinish() const
{
	return CSoundScene2D::GetCurTime() - GetBeginTime() >= timeToPlay;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int CSound::GetSamplesPassed()
{
	return bStartedMark ? ( CSoundScene2D::GetCurTime() - GetBeginTime() ) * GetSound()->GetSampleRate() / 1000 : 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float CSound::GetVolume( const NTimer::STime time, const float fDist ) const
{
	// сначала - затухание удаленного звука
	float fVolDim = 0.0f;
	if ( bDimMark ) 
		fVolDim = 1.0f * (time - timeBeginDim) / SSoundSceneConsts::SS_SOUND_DIM_TIME;
	if ( fVolDim > 1.0f )
		return 0.0f;

	// теперь зависимость от расстояния
	float fVolDist = 0.0f;
	if ( fDist / SSoundSceneConsts::SS_SOUND_CELL_SIZE / SSoundSceneConsts::SS_TILE_SIZE > nMinRadius )
		fVolDist = 1.0f * ( fDist - SSoundSceneConsts::SS_SOUND_CELL_SIZE * SSoundSceneConsts::SS_TILE_SIZE * nMinRadius ) / ( nMaxRadius - nMinRadius ) / SSoundSceneConsts::SS_SOUND_CELL_SIZE / SSoundSceneConsts::SS_TILE_SIZE;
	if ( fVolDist > 1.0f )
		return 0.0f;

	return ( 1.0f - fVolDim ) * ( 1.0f - fVolDist );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSound::MarkToDim( const NTimer::STime time )
{
	timeBeginDim = time;
	bDimMark = true;
	wID = 0; // звук теперь управляется сценой
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const NDb::SSoundDesc* CSound::GetDesc() const
{
	return pDesc;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSound::UnSubstitute()
{
	pSubstitute = 0;
	timeToPlay = pSample== 0 ? 0 : 1000 * pSample->GetLenght() / pSample->GetSampleRate();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSound::Substitute( CSubstSound *_pSubstitute, NTimer::STime nStartTime )
{
	pSubstitute = _pSubstitute;
	ISound * pSound = pSubstitute->GetSound();
	int nSampleRate = pSound->GetSampleRate();
	timeBegin = nStartTime;
	timeToPlay = pSound == 0 ? 0 : 1000 * pSound->GetLenght() / nSampleRate;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CSound::GetRadiusMax() const
{
	return nMaxRadius * 2;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSound::SetPos( const CVec3 &_vPos )
{
	int nTime = CSoundScene2D::GetCurTime() - timeLastPosUpdate;
	if ( nTime != 0 )
		vSpeed = ( _vPos - vPos ) / nTime;
	vPos = _vPos;
	timeLastPosUpdate = CSoundScene2D::GetCurTime();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSound::SetSpeed( const CVec3 &_vSpeed )
{
	vSpeed = _vSpeed;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSound::SetBeginTime( const NTimer::STime time )
{
	timeBegin = time;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSound::MarkStarted()
{
	bStartedMark = true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSubstSound * CSound::GetSubst()
{
	return pSubstitute;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ISound * CSound::GetSound()
{
	if ( pSubstitute )
		return pSubstitute->GetSound();
	return pSample;
}
