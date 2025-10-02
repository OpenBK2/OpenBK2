#include "StdAfx.h"

#include "3dlib/ggeometry.h"
#include "WeatherVisual.h"
#include "Scene.h"
#include "TerraGen.h"
#include "Misc/Win32Random.h"
#include "System/FastMath.h"
#include "System/Commands.h"
#include "Sound/SoundScene.h"
#include "System/db.h"
#include "3DMotor/Gfx.h"
#include "3DMotor/GAnimLight.h"
#include "3DMotor/GScene.h"
#include "3DMotor/GSceneUtils.h"

#include "FullScreenFader.h"

#define DEF_VIS_PATCH_SIZE ( AI2Vis(AI_TILES_IN_PATCH * AI_TILE_SIZE) )

namespace NWeather
{
	static float s_fEffectsSpeedCoeff = 1.0f;
	static float s_fWndAffectsCoeff = 0.05f;
}

void CWeatherVisual::CWeatherPart::Recalc()
{
	if ( pValue == 0 )
		pValue = new NGScene::CObjectInfo;

	if ( nNumPart < 0 )
		return;

	pWeather.Refresh();
	vector<NGScene::CObjectInfo::SData> *pVisualData = pWeather->GetVisualData();
	if ( nNumPart >= pVisualData->size() || (*pVisualData)[nNumPart].verts.empty() )
		pValue->Clear();
	else
		pValue->AssignFast( &(*pVisualData)[nNumPart] );
}

CWeatherVisual::CWeatherVisual( const NDb::SWeatherDesc *_pWeatherDesc, CFuncBase<STime> *_pTimer, IFullScreenFader *_pScreenFader, 
							 CDBPtr<NDb::SAmbientLight> _pNormalWeatherLight, CDBPtr<NDb::SAmbientLight> _pBadWeatherLight )
	: pDesc( _pWeatherDesc ),
	pTimer( _pTimer ),
	bActive( false ),
	//bExistLightning( false ),
	eState( NDb::EWS_UNKNOWN ),
	pScreenFader( _pScreenFader ),
	pNormalWeatherLight( _pNormalWeatherLight ),
	pBadWeatherLight( _pBadWeatherLight )	
{	
	InitParts();
}

NGScene::IGameView::SMeshInfo CWeatherVisual::MakeMeshInfo()
{
	NGScene::IGameView::SMeshInfo meshInfo;

	nVisualParts = 0;
	for ( int i = 0; i < pDesc->partMaterials.size(); ++i )
	{
		if ( pDesc->partMaterials[i] )
		{
			meshInfo.parts.push_back( NGScene::IGameView::SPartInfo( new CWeatherPart( this, i ), Scene()->GetGView()->CreateMaterial( pDesc->partMaterials[i] ) ) );
			++nVisualParts;
		}
	}

	if ( nVisualParts == 0 && pDesc->pPartMaterial )
	{
		meshInfo.parts.push_back( NGScene::IGameView::SPartInfo( new CWeatherPart( this, 0 ), Scene()->GetGView()->CreateMaterial( pDesc->pPartMaterial ) ) );
		nVisualParts = 1;
	}
	return meshInfo;
}

void CWeatherVisual::InitParts()
{
	if ( !IsValid(pLightningColor) )
		pLightningColor = new NGScene::CCVec3( CVec3(0, 0, 0) );

	nVisualParts = 0;
	parts.resize( pDesc->nIntensity );
	partsPresent.resize( pDesc->nIntensity, true );

	for ( int i = 0; i < parts.size(); ++i )
	{
		parts[i].z = NWin32Random::Random( 0.0f, pDesc->fFallHeight );
		parts[i].x = NWin32Random::Random( 0.0f, DEF_VIS_PATCH_SIZE );
		parts[i].y = NWin32Random::Random( 0.0f, DEF_VIS_PATCH_SIZE );
	}

	vWindOffset = VNULL2;
	const NDb::STerrain *pTerraDesc = Scene()->GetTerraManager()->GetDesc();
	if ( pTerraDesc )
	{
		vWindOffset = GetVectorByDirection( pTerraDesc->weather.nWindDirection );

		if ( pDesc->bWindAffected )
		{
			if ( pTerraDesc->weather.nWindForce )
				vWindOffset *= pTerraDesc->weather.nWindForce * NWeather::s_fWndAffectsCoeff;
		}

		UpdateAreas();
	}

	UpdateViewNormal();

	pTimer.Refresh();	
	timeLightStop = pTimer->GetValue() + 1000*60/(pDesc->fLightningsPerMinute + NWin32Random::Random( -pDesc->fLightningsRandom, pDesc->fLightningsRandom ));
	timeThunder = 0;

	//fWindForce = Scene()->GetWindController()->GetWindIntensity();
	//fWindForce += NWeather::s_fWndOver;
	//fWindForce = ClampFast( fWindForce, 0.0f, NWeather::s_fWndMaxIntensity );

	if ( !pDesc->lightnings.empty() )
	{
		lightningLoaders.resize( pDesc->lightnings.size() );
		
		for ( int iLightning = 0; iLightning < pDesc->lightnings.size(); ++iLightning )
		{
			if ( !pDesc->lightnings[iLightning] )
				continue;

			const NDb::SLightInstance *pInstance = pDesc->lightnings[iLightning];
			CDBPtr<NDb::SAnimLight> pLight = pInstance->pLight;

			NGScene::CLightLoader *pLoader = new NGScene::CLightLoader();
			pLoader->SetKey( pLight );

			lightningLoaders[iLightning] = pLoader;
			lightningLoaders[iLightning].Refresh();
			lightningLoaders[iLightning]->GetValue();
		}
	}
}

void CWeatherVisual::Recalc()
{
	if ( !pTimer )
		return;

	if ( !nVisualParts )
		return;

	visualData.resize( nVisualParts );
	visualData.assign( nVisualParts, NGScene::CObjectInfo::SData() );

	const NTimer::STime currTime = pTimer->GetValue();

	const float fTimeDelta = (currTime - timeLastRecalc) * NWeather::s_fEffectsSpeedCoeff;
	timeLastRecalc = currTime;

	// update drawing areas in case of camera has been moved
	if ( WasCameraMoved() )
	{
		UpdateAreas();
		UpdateViewNormal();
	}

	int nTotalPartCount = 0;
	fFadeCoeff = 0;

	switch ( eState )
	{
		case NDb::EWS_CLEAR:
		{
			fFadeCoeff = 0.0f;
			break;
		}
		//
		case NDb::EWS_BAD:
		{
			fFadeCoeff = 1.0f;
			break;
		}
		//
		case NDb::EWS_FADE_IN:
		{
			if ( timeLength > 0 )
				fFadeCoeff = ClampFast( (timeLastRecalc - timeStart)/(float)timeLength, 0.0f, 1.0f );
			else
				fFadeCoeff = 1.0;
			if ( fFadeCoeff == 1.0 )
				eState = NDb::EWS_BAD;
			break;
		}
		//
		case NDb::EWS_FADE_OUT:
		{
			if ( timeLength > 0 )
				fFadeCoeff = 1.0f - ClampFast( (timeLastRecalc - timeStart)/(float)timeLength, 0.0f, 1.0f );
			if ( fFadeCoeff == 0.0 )
			{
				eState = NDb::EWS_CLEAR;
				bActive = false;
			}

			break;
		}
	}

	if ( pLightningAnimator )
	{
		pLightningAnimator.Refresh();
		const NGScene::CAnimLight *pLightValue = pLightningAnimator->GetValue();
		if ( pLightValue->bEnd )
		{
			pLightningAnimator = 0;
			if ( IsValid( pScreenFader ) )
				pScreenFader->RemoveColorModificator( pLightningColor );
		}
		else if ( pLightValue->bActive )
		{
			const NGScene::CAnimLight *pValue = pLightningAnimator->GetValue();

			if ( pValue )
				pLightningColor->Set( pValue->color );
		}
	}

	if ( (pDesc->eType == NDb::WEATHER_RAIN) &&	//
			 (fFadeCoeff > 0.9f) &&									// no lightning in the beginning of rain
			 !pDesc->lightnings.empty() && bActive )					//
	{
		// do we need to play thunder sound
		if ( (timeThunder > 0) && (timeThunder <= timeLastRecalc) )
		{
			if ( !pDesc->randomSounds.empty() )
			{
				const NDb::SComplexSoundDesc *pSound = pDesc->randomSounds[NWin32Random::Random(pDesc->randomSounds.size())];
				if ( pSound )
					SoundScene()->AddSound( pSound, VNULL3, SFX_INTERFACE, SAM_ADD_N_FORGET, 0, 1.0f );
			}
			timeThunder = 0;
		}

		if ( !pLightningAnimator )
		{
			// there is no lightning at this moment: we can generate it
			if ( timeLightStop < timeLastRecalc )
			{
				int nRandom = NWin32Random::Random(0, pDesc->lightnings.size() );
				nRandom = Clamp( nRandom, 0, pDesc->lightnings.size() - 1 );

				const NDb::SLightInstance *pInstance = pDesc->lightnings[nRandom];
				if ( pInstance )
				{
					NGScene::CLightAnimator *pAnimator = new NGScene::CLightAnimator( pInstance, pTimer->GetValue(), pInstance->fScale );
					pAnimator->pInfo = lightningLoaders[nRandom];
					pAnimator->pTime = pTimer;
					SFBTransform transform;
					Identity( &transform.forward );
					Identity( &transform.backward );
					pAnimator->pPlacement = new CCSFBTransform( transform );

					pLightningAnimator = pAnimator;
					pLightningColor->Set( CVec3( 0, 0, 0) );
					if ( IsValid(pScreenFader) )
						pScreenFader->AddColorModificator( pLightningColor, false );

					timeThunder = timeLastRecalc + NWin32Random::Random( 100, 1000 );					
				}

				timeLightStop = timeLastRecalc + 1000*60/(pDesc->fLightningsPerMinute + NWin32Random::Random( -pDesc->fLightningsRandom, pDesc->fLightningsRandom ));
			}
		}
	}

	for ( int i = 0; i < parts.size() * fFadeCoeff; ++i )
	{
		if ( !partsPresent[i] )	// "The particle has left the building"
			continue;

		// Calculate everything
		switch ( pDesc->eType )
		{
			case NDb::WEATHER_RAIN:
			case NDb::WEATHER_SNOW:
			{
				if ( parts[i].z > 0.0f )
					parts[i].z -= pDesc->fSpeed * fTimeDelta;
				else if ( bActive )
				{
					while ( parts[i].z <= 0.0f )
						parts[i].z += pDesc->fFallHeight;

					parts[i].x = NWin32Random::Random( 0.0f, DEF_VIS_PATCH_SIZE );
					parts[i].y = NWin32Random::Random( 0.0f, DEF_VIS_PATCH_SIZE );
				}
				else
				{
					partsPresent[i] = false;
					continue;
				}
			}
			break;
			//
			case NDb::WEATHER_SANDSTORM:
			{
				parts[i].x += vWindOffset.x * pDesc->fSpeed * fTimeDelta;
				if ( parts[i].x < 0 )
					parts[i].x += DEF_VIS_PATCH_SIZE;
				if ( parts[i].x > DEF_VIS_PATCH_SIZE )
					parts[i].x -= DEF_VIS_PATCH_SIZE;

				parts[i].y += vWindOffset.y * pDesc->fSpeed * fTimeDelta;
				if ( parts[i].y < 0 )
					parts[i].y += DEF_VIS_PATCH_SIZE;
				if ( parts[i].y > DEF_VIS_PATCH_SIZE )
					parts[i].y -= DEF_VIS_PATCH_SIZE;

				if ( bActive )
				{
					parts[i].z = AI2Vis( Scene()->GetTerraManager()->GetZ(Vis2AI(parts[i].x), Vis2AI(parts[i].y)) );
					parts[i].z +=	pDesc->fFallHeight * ( 2.0f +	NMath::Sin(0.002f * timeLastRecalc + i + parts[i].x) );
				}
				else
					parts[i].z -= pDesc->fSpeed * fTimeDelta * 0.1f;

				if ( !bActive && parts[i].z <= 0.0f )
				{
					partsPresent[i] = false;
					continue;
				}
			}
			break;
			//
			default:
			{
				partsPresent[i] = false;
				continue;
			}
		}

		++nTotalPartCount;
	}

	if ( nTotalPartCount == 0 )
	{		
		return;
	}

	nTotalPartCount *= ( vBBMax.x - vBBMin.x ) * ( vBBMax.y - vBBMin.y );

	for ( int i = 0; i < nVisualParts; ++i )
	{
		NGScene::CObjectInfo::SData &data = visualData[i];
		data.verts.resize( 4 * nTotalPartCount );
		data.geometry.resize( 2 * nTotalPartCount );
	}

	static vector<int> partsIndex;
	partsIndex.resize( nVisualParts );
	partsIndex.assign( nVisualParts, 0 );
	
	// We shouln't draw particles in the black border of the map
	const NDb::STerrain *pTerraDesc = Scene()->GetTerraManager()->GetDesc();
	CVec2 vMinCoords( 4*VIS_TILE_SIZE, 4*VIS_TILE_SIZE );
	CVec2 vMaxCoords( pTerraDesc->nNumPatchesX*DEF_VIS_PATCH_SIZE - 4*VIS_TILE_SIZE, pTerraDesc->nNumPatchesY*DEF_VIS_PATCH_SIZE - 4*VIS_TILE_SIZE );	

	DWORD dwColor = NGfx::GetDWORDColor( GetParticlesColor() );

	// Draw one for each patch
	for ( int i = 0; i < parts.size() * fFadeCoeff; ++i )
	{
		if ( !bActive || !partsPresent[i] )
			continue;
		
		int nNumPart = i % nVisualParts;
		NGScene::CObjectInfo::SData &data = visualData[nNumPart];

		for ( int nY = vBBMin.y; nY < vBBMax.y; ++nY )
		{
			for ( int nX = vBBMin.x; nX < vBBMax.x; ++nX )
			{
				CVec3 vPos( parts[i] );
				vPos.x += nX * DEF_VIS_PATCH_SIZE;
				vPos.y += nY * DEF_VIS_PATCH_SIZE;

				const int nIndex2 = partsIndex[nNumPart] * 2;
				const int nIndex4 = partsIndex[nNumPart] * 4;

				switch ( pDesc->eType )
				{
				case NDb::WEATHER_RAIN:
					{
						if ( pDesc->bWindAffected )
						{
							vPos.x += vPos.z * vWindOffset.x;
							vPos.y += vPos.z * vWindOffset.y;
						}

						if ( vPos.x <= vMinCoords.x || vPos.y <= vMinCoords.y ||
							vPos.x >= vMaxCoords.x || vPos.y >= vMaxCoords.y )
							continue;

						CVec3 vPos2( vPos );
						float fHeight = AI2Vis(Scene()->GetZ( Vis2AI(vPos2.x), Vis2AI(vPos2.y) ));
						if ( vPos.z < fHeight )
							continue;

						vPos2.z = Max( vPos.z - pDesc->fTrajectoryParameter, fHeight );

						if ( pDesc->bWindAffected )
						{
							vPos2.x += ( vPos2.z - vPos.z ) * vWindOffset.x;
							vPos2.y += ( vPos2.z - vPos.z ) * vWindOffset.y;
						}

						data.verts[nIndex4 + 0].pos = vPos;
						data.verts[nIndex4 + 1].pos = vPos2;
					}
					break;
					//
				case NDb::WEATHER_SNOW:
					{
						if ( pDesc->bWindAffected )
						{
							vPos.x += vPos.z * vWindOffset.x;
							vPos.y += vPos.z * vWindOffset.y;
						}

						if ( vPos.x <= vMinCoords.x || vPos.y <= vMinCoords.y ||
							vPos.x >= vMaxCoords.x || vPos.y >= vMaxCoords.y )
							continue;

						CVec3 vPos2( vPos );
						vPos2.x += NMath::Sin( vPos.z / (i % 3 + 3) + vPos.x ) * pDesc->fTrajectoryParameter;
						vPos2.y += NMath::Sin( vPos.z / (i % 3 + 3) + vPos.y ) * pDesc->fTrajectoryParameter;

						float fHeight = AI2Vis(Scene()->GetZ( Vis2AI(vPos2.x), Vis2AI(vPos2.y) ));
						if ( vPos2.z < fHeight )
							continue;

						data.verts[nIndex4 + 0].pos = vPos2;
						data.verts[nIndex4 + 1].pos = vPos2 - vViewNormal3 * pDesc->fPartSize;
					}
					break;
					//
				case NDb::WEATHER_SANDSTORM:
					{
						CVec3 vPos2( vPos );

						float fHeight = AI2Vis(Scene()->GetZ( Vis2AI(vPos2.x), Vis2AI(vPos2.y) ));
						vPos2.z = Max( vPos.z - pDesc->fTrajectoryParameter,fHeight );

						data.verts[nIndex4 + 0].pos = vPos2;
						//data.verts[nIndex4 + 1].pos = vPos2 - vViewNormal * pDesc->fPartSize;
						data.verts[nIndex4 + 1].pos = vPos2 - vViewNormal3 * pDesc->fPartSize;
					}
					break;
				}
				const float fZoomCoeff = AI2Vis( Camera()->GetDistance() ) / 10.0f;
				//
				data.verts[nIndex4 + 2].pos = data.verts[nIndex4 + 0].pos - vViewNormal * pDesc->fPartSize * fZoomCoeff;
				data.verts[nIndex4 + 3].pos = data.verts[nIndex4 + 1].pos - vViewNormal * pDesc->fPartSize * fZoomCoeff;

				data.verts[nIndex4 + 0].tex = CVec2( 0, 0 );
				data.verts[nIndex4 + 1].tex = CVec2( 0, 1 );
				data.verts[nIndex4 + 2].tex = CVec2( 1, 0 );
				data.verts[nIndex4 + 3].tex = CVec2( 1, 1 );

				CalcCompactVector( &(data.verts[nIndex4 + 0].normal), V3_AXIS_Z );
				CalcCompactVector( &(data.verts[nIndex4 + 1].normal), V3_AXIS_Z );
				CalcCompactVector( &(data.verts[nIndex4 + 2].normal), V3_AXIS_Z );
				CalcCompactVector( &(data.verts[nIndex4 + 3].normal), V3_AXIS_Z );

				data.verts[nIndex4 + 0].texU.dw = dwColor;
				data.verts[nIndex4 + 0].texV.dw = dwColor;
				data.verts[nIndex4 + 1].texU.dw = dwColor;
				data.verts[nIndex4 + 1].texV.dw = dwColor;
				data.verts[nIndex4 + 2].texU.dw = dwColor;
				data.verts[nIndex4 + 2].texV.dw = dwColor;
				data.verts[nIndex4 + 3].texU.dw = dwColor;
				data.verts[nIndex4 + 3].texV.dw = dwColor;

				data.geometry[nIndex2].i1 = nIndex4 + 0;
				data.geometry[nIndex2].i2 = nIndex4 + 1;
				data.geometry[nIndex2].i3 = nIndex4 + 2;
				data.geometry[nIndex2 + 1].i1 = nIndex4 + 1;
				data.geometry[nIndex2 + 1].i2 = nIndex4 + 2;
				data.geometry[nIndex2 + 1].i3 = nIndex4 + 3;

				++partsIndex[nNumPart];

			}
		}
	}

	for ( int i = 0; i < nVisualParts; ++i )
	{
		NGScene::CObjectInfo::SData &data = visualData[i];
		data.verts.resize( 4 * partsIndex[i] );
		data.geometry.resize( 2 * partsIndex[i] );
	}
}


void CWeatherVisual::UpdateAreas()
{
	const NDb::STerrain *pTerraDesc = Scene()->GetTerraManager()->GetDesc();

	vBBMin.Set( pTerraDesc->nNumPatchesX, pTerraDesc->nNumPatchesY );
	vBBMax.Set( 0, 0 );

	CVec3 vZero;
	CVec2 vScreenSize = Scene()->GetScreenRect();

	// Top left - ( 0, 0 )
	Scene()->PickZeroHeight( &vZero, CVec2(0, 0) );
	vZero /= AI_TILES_IN_PATCH * AI_TILE_SIZE;
	vBBMin.Minimize( CVec2(vZero.x, vZero.y) );
	vBBMax.Maximize( CVec2(vZero.x, vZero.y) );

	// Top right - ( 1024, 0 )
	Scene()->PickZeroHeight( &vZero, CVec2(vScreenSize.x, 0) );
	vZero /= AI_TILES_IN_PATCH * AI_TILE_SIZE;
	vBBMin.Minimize( CVec2(vZero.x, vZero.y) );
	vBBMax.Maximize( CVec2(vZero.x, vZero.y) );

	// Bottom left - ( 0, 768 )
	Scene()->PickZeroHeight( &vZero, CVec2(0, vScreenSize.y) );
	vZero /= AI_TILES_IN_PATCH * AI_TILE_SIZE;
	vBBMin.Minimize( CVec2(vZero.x, vZero.y) );
	vBBMax.Maximize( CVec2(vZero.x, vZero.y) );

	// Bottom right - ( 1024, 768 )
	Scene()->PickZeroHeight( &vZero, CVec2(vScreenSize.x, vScreenSize.y) );
	vZero /= AI_TILES_IN_PATCH * AI_TILE_SIZE;
	vBBMin.Minimize( CVec2(vZero.x, vZero.y) );
	vBBMax.Maximize( CVec2(vZero.x, vZero.y) );

	//vBBMin -= CVec2( 1, 1 );
	vBBMax += CVec2( 1, 1 );

	vBBMin.Set( Clamp((int)vBBMin.x, 0, pTerraDesc->nNumPatchesX), Clamp((int)vBBMin.y, 0, pTerraDesc->nNumPatchesY) );
	vBBMax.Set( Clamp((int)vBBMax.x, 0, pTerraDesc->nNumPatchesX), Clamp((int)vBBMax.y, 0, pTerraDesc->nNumPatchesY) );
}

static inline float Lerp( float fFactor, float fA, float fB ) { return fA*(1-fFactor) + fB*fFactor; }

void CWeatherVisual::UpdateLights( float fFadeCoeff )
{
	if ( !pNormalWeatherLight || !pBadWeatherLight )
		return;

	const float fEpsilon = 0.01f; // Calculation error
	if ( fFadeCoeff < fEpsilon )
	{
		if ( pCurrentWeatherLight )
			Scene()->GetGView()->SetAmbient( pNormalWeatherLight );
		pCurrentWeatherLight = 0;
		return;
	}

	if ( fFadeCoeff > 1.0f - fEpsilon )
		return;

	if ( !IsValid( pCurrentWeatherLight ) )
		pCurrentWeatherLight = pNormalWeatherLight->Duplicate();

	pCurrentWeatherLight->vLightColor.Lerp( fFadeCoeff, pNormalWeatherLight->vLightColor, pBadWeatherLight->vLightColor );
	pCurrentWeatherLight->vAmbientColor.Lerp( fFadeCoeff, pNormalWeatherLight->vAmbientColor, pBadWeatherLight->vAmbientColor );
	pCurrentWeatherLight->vShadeColor.Lerp( fFadeCoeff, pNormalWeatherLight->vShadeColor, pBadWeatherLight->vShadeColor );
	pCurrentWeatherLight->vIncidentShadowColor.Lerp( fFadeCoeff, pNormalWeatherLight->vIncidentShadowColor, pBadWeatherLight->vIncidentShadowColor );
	pCurrentWeatherLight->vParticlesColor.Lerp( fFadeCoeff, pNormalWeatherLight->vParticlesColor, pBadWeatherLight->vParticlesColor );

	Scene()->GetGView()->SetAmbient( pCurrentWeatherLight );
}

void CWeatherVisual::UpdateAmbientSound()
{
	if ( !pAbsTimer )
	{
		pAbsTimer = Scene()->GetAbsTimer();
		PlayAmbientSound();
	}

	if ( !pAbsTimer.Refresh() )
		return;

	// Ambient sound
	if ( fFadeCoeff > 0.9f && timeNextAmbientSound < pAbsTimer->GetValue() )
		PlayAmbientSound();
}

void CWeatherVisual::PlayAmbientSound()
{
	if ( !pAbsTimer )
		pAbsTimer = Scene()->GetAbsTimer();

	pAbsTimer.Refresh();

	if ( pDesc->ambientSound.empty() )
		return;

	const int nSound = Clamp( NWin32Random::Random(0, pDesc->ambientSound.size()), 0, pDesc->ambientSound.size()-1 );

	const NDb::SComplexSoundDesc *pSound =  pDesc->ambientSound[nSound].pAmbientSound;
	if ( pSound )
		SoundScene()->AddSound( pSound, VNULL3, SFX_INTERFACE, SAM_ADD_N_FORGET, 0, 1.0f );

	timeNextAmbientSound = pAbsTimer->GetValue() + pDesc->ambientSound[nSound].fSoundLength*1000;
}

CVec4 CWeatherVisual::GetParticlesColor()
{
	if ( pCurrentWeatherLight )
		return CVec4( pCurrentWeatherLight->vParticlesColor * 4, 1 );

	if ( pNormalWeatherLight )
		return CVec4( pNormalWeatherLight->vParticlesColor * 4, 1 );

	if ( pBadWeatherLight )
		return CVec4( pBadWeatherLight->vParticlesColor * 4, 1 );

	return CVec4( 1, 1, 1, 1 );
}

void CWeatherVisual::Update()
{
	UpdateLights( fFadeCoeff );
}

void CWeatherVisual::SwitchOn( NTimer::STime _timeLength )
{
	timeLength = _timeLength;
	pTimer.Refresh();
	timeStart = pTimer->GetValue();
	timeLastRecalc = timeStart;
	timeLightStop = pTimer->GetValue() + 1000*60/(pDesc->fLightningsPerMinute + NWin32Random::Random( -pDesc->fLightningsRandom, pDesc->fLightningsRandom ));
	timeThunder = 0;
	PlayAmbientSound();

	eState = NDb::EWS_FADE_IN;
	bActive = true;
}


void CWeatherVisual::SwitchOff( NTimer::STime _timeLength )
{
	timeLength = _timeLength;
	pTimer.Refresh();
	timeStart = pTimer->GetValue();
	timeLastRecalc = timeStart;

	eState = NDb::EWS_FADE_OUT;

	if ( IsValid( pScreenFader ) )
		pScreenFader->RemoveColorModificator( pLightningColor );

}


int CWeatherVisual::operator&( IBinSaver &saver )
{
	saver.Add( 2, &timeStart );
	saver.Add( 3, &timeLength );
	saver.Add( 4, &timeLastRecalc );
	saver.Add( 5, &pTimer );
	saver.Add( 6, &pDesc );
	saver.Add( 9, &bActive );
	//saver.Add( 17, &bExistLightning );
	//saver.Add( 18, &bThunder );
	saver.Add( 22, &eState );
	saver.Add( 26, &pNormalWeatherLight );
	saver.Add( 27, &pBadWeatherLight );

	if ( saver.IsReading() )
	{
		InitParts();		
	}

	return 0;
}


START_REGISTER( Weather )
	REGISTER_VAR_EX( "weather_speed_coeff", NGlobal::VarFloatHandler, &NWeather::s_fEffectsSpeedCoeff, 1.0f, STORAGE_NONE )
	REGISTER_VAR_EX( "weather_wnd_affects_coeff", NGlobal::VarFloatHandler, &NWeather::s_fWndAffectsCoeff, 0.05f, STORAGE_NONE )
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( 0x1B1A5C80, CWeatherVisual )


