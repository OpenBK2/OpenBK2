#include "stdafx.h"

#include "DebugTools/DebugInfoManagerInternal.h"
#include "Misc/HashFuncs.h"
#include "SceneInternal.h"
#include "DBSceneConsts.h"

static std::unordered_map<NDebugInfo::EColor, CDBPtr<NDb::SMaterial>, SEnumHash> colorsMap;

const NDb::SMaterial *Color2Color( const NDebugInfo::EColor color )
{
	std::unordered_map<NDebugInfo::EColor, CDBPtr<NDb::SMaterial>, SEnumHash >::const_iterator pos = colorsMap.find( color );
	if ( pos == colorsMap.end() )
		return 0;
	else
		return pos->second;
}

void CScene::InitDebugMaterials( const NDb::SSceneConsts *pSceneConsts )
{
	colorsMap.clear();
	colorsMap[NDebugInfo::RED] = pSceneConsts->debugMaterials.pRedMaterial;
	colorsMap[NDebugInfo::GREEN] = pSceneConsts->debugMaterials.pGreenMaterial;
	colorsMap[NDebugInfo::BLUE] = pSceneConsts->debugMaterials.pBlueMaterial;
	colorsMap[NDebugInfo::BLACK] = pSceneConsts->debugMaterials.pBlackMaterial;
	colorsMap[NDebugInfo::WHITE] = pSceneConsts->debugMaterials.pWhiteMaterial;
}

void CScene::AddDebugInfoObject( const int nID, NAIVisInfo::CDebugObject *pObject )
{
	debugInfoObjects[nID] = pObject;
}

void CScene::CreateMarker( const NDebugInfo::SDebugInfoMarker *pMarker )
{
	NAIVisInfo::CDebugMarkers *pMarkers = new NAIVisInfo::CDebugMarkers();
	pMarkers->Init( pMarker->tiles );
	pMarkers->SetColor( Color2Color( pMarker->color ) );

	AddDebugInfoObject( pMarker->nID, pMarkers );
}

void CScene::CreateCircle( const NDebugInfo::SDebugInfoCircle *pCircle )
{
	NAIVisInfo::CDebugCircle *pCircleObj = new NAIVisInfo::CDebugCircle();
	pCircleObj->Init( pCircle->circle );
	pCircleObj->SetColor( Color2Color( pCircle->color ) );

	return AddDebugInfoObject( pCircle->nID, pCircleObj );
}

void CScene::CreateSegment( const NDebugInfo::SDebugInfoSegment *pSegment )
{
	NAIVisInfo::CDebugSegment *pSegmentObj = new NAIVisInfo::CDebugSegment();
	pSegmentObj->Init( pSegment->segment, pSegment->nThickness );
	pSegmentObj->SetColor( Color2Color( pSegment->color ) );

	return AddDebugInfoObject( pSegment->nID, pSegmentObj );
}

void CScene::CreateLine( const NDebugInfo::SDebugInfoLine *pLine )
{
	std::vector<CVec3> points;
	std::vector<WORD> indices;
	const WORD wEndArrowIndex = ( pLine->arrowStart.fHeight != 0.0f ) ? 6 : 2;

	points.push_back( pLine->arrowStart.vPosition );
	points.push_back( pLine->arrowEnd.vPosition );
	if ( pLine->arrowStart.fHeight != 0.0f || pLine->arrowEnd.fHeight != 0.0f )
	{
		CVec3 vDir = pLine->arrowEnd.vPosition-pLine->arrowStart.vPosition;
		Normalize( &vDir );

		CVec3 v1 = ( vDir.y == 0.0f && vDir.z == 0.0f ) ? CVec3( 0.0f, 1.0f, 0.0f ) : vDir ^ CVec3( 1.0f, 0.0f, 0.0f ) ;
		Normalize( &v1 );
		CVec3 v2 = vDir ^ v1;
		Normalize( &v2 );
		if ( pLine->arrowStart.fHeight != 0.0f )
		{
			points.push_back( pLine->arrowStart.vPosition + vDir * pLine->arrowStart.fWidth + v1 * pLine->arrowStart.fHeight );
			points.push_back( pLine->arrowStart.vPosition + vDir * pLine->arrowStart.fWidth - v1 * pLine->arrowStart.fHeight );
			points.push_back( pLine->arrowStart.vPosition + vDir * pLine->arrowStart.fWidth + v2 * pLine->arrowStart.fHeight );
			points.push_back( pLine->arrowStart.vPosition + vDir * pLine->arrowStart.fWidth - v2 * pLine->arrowStart.fHeight );
			indices.push_back( 2 );
			indices.push_back( 0 );
			indices.push_back( 4 );
			indices.push_back( 0 );
			indices.push_back( 5 );
			indices.push_back( 0 );
			indices.push_back( 3 );
		}

		if ( pLine->arrowEnd.fHeight != 0.0f )
		{
			points.push_back( pLine->arrowEnd.vPosition - vDir * pLine->arrowEnd.fWidth + v1 * pLine->arrowEnd.fHeight );
			points.push_back( pLine->arrowEnd.vPosition - vDir * pLine->arrowEnd.fWidth - v1 * pLine->arrowEnd.fHeight );
			points.push_back( pLine->arrowEnd.vPosition - vDir * pLine->arrowEnd.fWidth + v2 * pLine->arrowEnd.fHeight );
			points.push_back( pLine->arrowEnd.vPosition - vDir * pLine->arrowEnd.fWidth - v2 * pLine->arrowEnd.fHeight );
		}
	}
	indices.push_back( 0 );
	indices.push_back( 1 );
	if ( pLine->arrowEnd.fHeight != 0.0f )
	{
		indices.push_back( wEndArrowIndex );
		indices.push_back( 1 );
		indices.push_back( wEndArrowIndex + 1 );
		indices.push_back( 1 );
		indices.push_back( wEndArrowIndex + 2 );
		indices.push_back( 1 );
		indices.push_back( wEndArrowIndex + 3 );
		indices.push_back( 1 );
	}
	AddIndexedPolyline( pLine->nID, points, indices, pLine->vColor, pLine->bDepthCheck );
}

void CScene::CreateRect( const NDebugInfo::SDebugInfoRect *pRect )
{
	std::vector<CVec3> points;
	std::vector<WORD> indices;
	points.push_back( CVec3( pRect->rect.v1, pRect->fZ ) );
	points.push_back( CVec3( pRect->rect.v2, pRect->fZ ) );
	points.push_back( CVec3( pRect->rect.v3, pRect->fZ ) );
	points.push_back( CVec3( pRect->rect.v4, pRect->fZ ) );
	indices.push_back( 0 );
	indices.push_back( 1 );
	indices.push_back( 1 );
	indices.push_back( 2 );
	indices.push_back( 2 );
	indices.push_back( 3 );
	indices.push_back( 3 );
	indices.push_back( 0 );
	AddIndexedPolyline( pRect->nID, points, indices, pRect->vColor, pRect->bDepthCheck );
}

void CScene::ProcessDebugInfoUpdates()
{
	CDebugInfoManager *pDebugInfoManager = static_cast<CDebugInfoManager *>( DebugInfoManager() );

	do {
		const NDebugInfo::SDebugInfoUpdate *pDebugInfoUpdate = pDebugInfoManager->GetUpdate();
		if ( pDebugInfoUpdate )
		{
			switch( pDebugInfoUpdate->GetDebugInfoUpdateID() ) {
			case NDebugInfo::DEBUG_INFO_MARKER:
				CreateMarker( static_cast<const NDebugInfo::SDebugInfoMarker *>(pDebugInfoUpdate) );
				break;
			case NDebugInfo::DEBUG_INFO_CIRCLE:
				CreateCircle( static_cast<const NDebugInfo::SDebugInfoCircle *>(pDebugInfoUpdate) );
				break;
			case NDebugInfo::DEBUG_INFO_SEGMENT:
				CreateSegment( static_cast<const NDebugInfo::SDebugInfoSegment *>(pDebugInfoUpdate) );
				break;
			case NDebugInfo::DEBUG_INFO_LINE:
				CreateLine( static_cast<const NDebugInfo::SDebugInfoLine *>(pDebugInfoUpdate) );
				break;
			case NDebugInfo::DEBUG_INFO_RECT:
				CreateRect( static_cast<const NDebugInfo::SDebugInfoRect *>(pDebugInfoUpdate) );
				break;
			case NDebugInfo::DEBUG_INFO_DELETE:
				{
					CDebugInfoObjects::iterator pos = debugInfoObjects.find( pDebugInfoUpdate->nID );
					if ( pos != debugInfoObjects.end() )
						debugInfoObjects.erase( pos );
				}
				break;
			case NDebugInfo::DEBUG_INFO_DELETE_LINE:
				RemovePolyline( pDebugInfoUpdate->nID );
				break;
			} 
		}
	} while( pDebugInfoManager->PopUpdate() );
}


