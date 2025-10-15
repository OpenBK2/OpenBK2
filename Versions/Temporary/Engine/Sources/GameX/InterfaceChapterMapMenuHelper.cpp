#include "stdafx.h"
#include "InterfaceChapterMapMenuHelper.h"
#include "UI/UI.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "DBScenario.h"

#include "GameX_export.h"

#include <cstdint>

// SChapterMapMenuHelper::SArrow

uint32_t SChapterMapMenuHelper::SArrow::GetColor() const
{
	return 0xffffffff;
}

uint32_t SChapterMapMenuHelper::SArrow::GetDependentColor() const
{
	return 0x40ffffff;
}

// SChapterMapMenuHelper

SChapterMapMenuHelper::SChapterMapMenuHelper( const NDb::SChapter *_pChapter, IWindow *pChapterMap )
{
	pChapter = _pChapter;
	pDetailsMap = pChapter->pDetailsMap;

	int nSizeX, nSizeY;
	pChapterMap->GetPlacement( 0, 0, &nSizeX, &nSizeY );
	
	vDetailsCoeff.Set( 1.0f, 1.0f );
	if ( pDetailsMap )
	{
		if ( nSizeX > 0 )
			vDetailsCoeff.x = float( nSizeX ) / ( pDetailsMap->nNumPatchesX * AI_TILE_SIZE * AI_TILES_IN_PATCH );
		if ( nSizeY > 0 )
			vDetailsCoeff.y = float( nSizeY ) / ( pDetailsMap->nNumPatchesY * AI_TILE_SIZE * AI_TILES_IN_PATCH );
	}
	
	missions.resize( pChapter->missionPath.size() );
	for ( int nMission = 0; nMission < pChapter->missionPath.size(); ++nMission )
	{
		const NDb::SMissionEnableInfo &missionDB = pChapter->missionPath[nMission];
		SMission &mission = missions[nMission];
		mission.nIndex = nMission;

		mission.fPotentialIncomplete = missionDB.fPotentialIncomplete;
		mission.fPotentialComplete = missionDB.fPotentialComplete;
		mission.bShowPotentialComplete = missionDB.bShowPotentialComplete;

		mission.vEndOffset = missionDB.vEndOffset;

		bool bFound = false;
		const NDb::SMapInfo *pDetailsMap = pChapter->pDetailsMap;
		if ( pDetailsMap )
		{
			// search for mission
			for ( int i = 0; i < pDetailsMap->objects.size(); ++i )
			{
				if ( pDetailsMap->objects[i].nPlayer == nMission )
				{
					mission.nObjectIndex = i;
					CVec2 vPos = Map2Screen( pDetailsMap->objects[i].vPos );
					mission.vPos = vPos;
					bFound = true;
					if ( nMission == 0 )
					{
						float fAngle = ToRadian( pChapter->fMainStrikeAngle );
						CVec2 vDir( cosf( fAngle ), sinf( fAngle) );
						vMainStrike = vDir * pChapter->fMainStrikePower;
					}
					break;
				}
			}
			
			// search for arrows
			mission.arrows.reserve( pDetailsMap->roads.size() );
			for ( int nRoad = 0; nRoad < pDetailsMap->roads.size(); ++nRoad )
			{
				const NDb::SVSOInstance &road = pDetailsMap->roads[nRoad];

				if ( road.nCMArrowMission/*points[0].fReserved*/ != nMission )
					continue;

				int nArrowIndex = road.nCMArrowType/*road.points[0].fRadius*/;
				if ( pChapter )
				{
					if ( nArrowIndex >= 0 && nArrowIndex < pChapter->arrowTextures.size() )
					{
						mission.arrows.push_back( SArrow() );
						SArrow &arrow = mission.arrows.back();

						arrow.vDelta = VNULL2;
						arrow.nID = nRoad;
						arrow.pTexture = pChapter->arrowTextures[nArrowIndex];

						arrow.rcBounds.SetEmpty();
						arrow.points.resize( road.points.size() );
						for ( int i = 0; i < road.points.size(); ++i )
						{
							arrow.points[i] = Map2Screen( road.points[i].vPos );
							arrow.rcBounds.Union( CTRect<float>( arrow.points[i].x, arrow.points[i].y, 
								arrow.points[i].x + 1, arrow.points[i].y + 1 ) );
						}
						arrow.fWidth = road.points[0].fWidth / 4;
						arrow.rcBounds.Inflate( arrow.fWidth * 0.5f, arrow.fWidth * 0.5f );

						arrow.nDependIndex = -1;
						if ( nMission == 0 )		// Analyze small missions, display only completed arrows
						{
							if ( road.nCMArrowMission2/*points[1].fReserved*/ > 0 )
							{
								arrow.nDependIndex = road.nCMArrowMission2/*points[1].fReserved*/;
							}
						}
					}
					else
						NI_ASSERT( 0, "Designers: not enough textures for chapter arrows" );
				}
			}
		}
		if ( !bFound )
		{
			mission.vPos = missionDB.vPlaceOnChapterMap;
		}
	}
}

CVec2 SChapterMapMenuHelper::Map2Screen( const CVec3 &vMapPos ) const
{
	return CVec2( vMapPos.y * vDetailsCoeff.x, vMapPos.x * vDetailsCoeff.y );
}

CVec3 SChapterMapMenuHelper::Screen2Map( const CVec2 &vScreenPos ) const
{
	return CVec3( vScreenPos.y / vDetailsCoeff.y, vScreenPos.x / vDetailsCoeff.x, 0.0f );
}

void SChapterMapMenuHelper::UpdateMission( NDb::SMapInfo *pDetailsMap, SMission *pMission )
{
	if ( pDetailsMap )
	{
		pDetailsMap->objects[pMission->nObjectIndex].vPos = Screen2Map( pMission->vPos );
	}
}

void SChapterMapMenuHelper::UpdateArrow( NDb::SMapInfo *pDetailsMap, SArrow *pArrow )
{
	if ( pDetailsMap )
	{
		NDb::SVSOInstance *pRoad = &pDetailsMap->roads[pArrow->nID];
		
		for ( int i = 0; i < pArrow->points.size(); ++i )
		{
			pRoad->points[i].vPos = Screen2Map( pArrow->points[i] );
		}
		for ( int i = 0; i < pRoad->controlPoints.size(); ++i )
		{
			CVec3 &vPos = pRoad->controlPoints[i];

			vPos = Screen2Map( Map2Screen( vPos ) + pArrow->vDelta );
		}
		pArrow->vDelta = VNULL2;

/*		if ( pRoad->points.size() >= 2 )
		{
			pRoad->nCMArrowType = pRoad->points[0].fRadius;
			pRoad->nCMArrowMission = pRoad->points[0].fReserved;
			pRoad->nCMArrowMission2 = pRoad->points[1].fReserved;
		}*/
	}
}

void SChapterMapMenuHelper::MoveArrow( SArrow *pArrow, const CVec2 &vDelta )
{
	pArrow->vDelta += vDelta;

	pArrow->rcBounds.SetEmpty();
	for ( int i = 0; i < pArrow->points.size(); ++i )
	{
		CVec2 &vPos = pArrow->points[i];

		vPos += vDelta;
		pArrow->rcBounds.Union( CTRect<float>( vPos.x, vPos.y, vPos.x + 1, vPos.y + 1 ) );
	}
	pArrow->rcBounds.Inflate( pArrow->fWidth * 0.5f, pArrow->fWidth * 0.5f );
}

void SChapterMapMenuHelper::ReReadPotentials()
{
	if ( pDetailsMap )
	{
		const int nMission = 0;
		const NDb::SMissionEnableInfo &missionDB = pChapter->missionPath[nMission];
		// search for mission
		for ( int i = 0; i < pDetailsMap->objects.size(); ++i )
		{
			if ( pDetailsMap->objects[i].nPlayer == nMission )
			{
				float fAngle = ToRadian( pChapter->fMainStrikeAngle );
				CVec2 vDir( cosf( fAngle ), sinf( fAngle) );
				vMainStrike = vDir * pChapter->fMainStrikePower;
				break;
			}
		}
	}
	for ( int nMission = 0; nMission < pChapter->missionPath.size(); ++nMission )
	{
		const NDb::SMissionEnableInfo &missionDB = pChapter->missionPath[nMission];
		SMission &mission = missions[nMission];

		mission.fPotentialIncomplete = missionDB.fPotentialIncomplete;
		mission.fPotentialComplete = missionDB.fPotentialComplete;
		mission.bShowPotentialComplete = missionDB.bShowPotentialComplete;
	}
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x1721A380, SChapterMapMenuHelper )


