#include "stdafx.h"

#include "3Dmotor/Gfx.h"
#include "3DLib/Transform.h"
#include "Camera.h"
#include "VisObjDesc.h"
#include "SceneInternal.h"

void CScene::PickTerrain( CVec3 *pvPos, const CVec2 &vScreenPos )
{
	CVec3 vNear, vFar;
	Camera()->GetProjectiveRayPoints( &vNear, &vFar, vScreenPos );
	Vis2AI( &vNear );
	Vis2AI( &vFar );
	GetIntersectionWithTerrain( pvPos, vNear, vFar );
}

void CScene::PickZeroHeight( CVec3 *pvPos, const CVec2 &vScreenPos )
{
	CVec3 vNear, vFar;
	Camera()->GetProjectiveRayPoints( &vNear, &vFar, vScreenPos );

	const float t = ( 0.0f - vNear.z)/( vFar.z - vNear.z );
	pvPos->x = (( vFar.x - vNear.x )*t + vNear.x);
	pvPos->y = (( vFar.y - vNear.y )*t + vNear.y);
	pvPos->z = 0.0f;
	Vis2AI( pvPos );
}

void CScene::PickAllObjects( const CVec3 &vAIPos1, const CVec3 &vAIPos2, std::list<SPickObjInfo> *pPickedObjects, std::list<int> *pPickedAttached )
{
	CVec3 vPos1, vPos2;
	AI2Vis( &vPos1, vAIPos1 );
	AI2Vis( &vPos2, vAIPos2 );

	const CVec3 vDir( vPos2 - vPos1 );
	if ( vDir == VNULL3 )
		return;

	CRay ray( vPos1, vDir );
	const float fLength = fabs( vDir );

	std::vector<NAI::SInterval> intervals;
	data[eScene]->pAIMap->Trace( ray, &intervals, 1 );
	for ( std::vector<NAI::SInterval>::const_iterator it = intervals.begin(); it != intervals.end(); ++it )
	{
		const NAI::SInterval &interval = *it;
		if ( interval.enter.fT > fLength )
			break;

		SPickObjInfo info;
		info.vPickPoint = vPos1 + vDir * interval.enter.fT;
		info.vNormal = interval.enter.ptNormal;

		CDynamicCast<const SVisObjDescBase> pVisObjDescBase = interval.pSrc->pUserData;

		if ( pVisObjDescBase )
		{
			const int nID = pVisObjDescBase->GetID();
			if ( (nID & 0x80000000) && data[eScene]->attachIDToMapObjID.find( nID ) != data[eScene]->attachIDToMapObjID.end() )
			{
				info.nObjID = data[eScene]->attachIDToMapObjID[nID];
				pPickedAttached->push_back( nID );
			}
			else
				info.nObjID = nID;

			pPickedObjects->push_back( info );
		}
	}
}

inline float GetGridPosition( float fPosition, int nGridSize )
{
	return (std::min)( (int)(fPosition*nGridSize), nGridSize-1 );
}

void CScene::PickObjects( std::list<int> &pickObjects, const CVec2 &vScreenPos, const EPickObjectsClass ePickObjsClass )
{
	if ( fastRender.resGrid.IsEmpty() )
		return;

	CVec3 vOrig, vDir;
	Camera()->GetProjectiveRay( &vOrig, &vDir, vScreenPos );
	if ( vDir == VNULL3 )
		return;

	CRay ray;
	ray.ptOrigin = vOrig;
	ray.ptDir = vDir;

	std::vector<NAI::SInterval> intervals;
	data[eScene]->pAIMap->Trace( ray, &intervals, 1 );
	for ( std::vector<NAI::SInterval>::const_iterator it = intervals.begin(); it != intervals.end(); ++it )
	{
		const NAI::SInterval &interval = *it;
		CDynamicCast<const SVisObjDescBase> pVisObjDescBase = interval.pSrc->pUserData;
		if ( pVisObjDescBase )
		{
			const int nID = pVisObjDescBase->GetID();

			if ( (nID & 0x80000000) && data[eScene]->attachIDToMapObjID.find( nID ) != data[eScene]->attachIDToMapObjID.end() )
				pickObjects.push_back( data[eScene]->attachIDToMapObjID[nID] );
			else
				pickObjects.push_back( nID );
		}
	}
	/*

	// Find grid node that is nearest to vScreenPos
	CVec2 vScreenRect = NGfx::GetScreenRect();
	CVec2 vPos( vScreenPos.x/vScreenRect.x, vScreenPos.y/vScreenRect.y );

	if ( vPos.x < 0.0f || vPos.x > 1.0f  ||
		vPos.y < 0.0f || vPos.y > 1.0f )
		return;

	int nX = GetGridPosition( vPos.x, fastRender.resGrid.GetSizeX() );
	int nY = GetGridPosition( (1 - vPos.y), fastRender.resGrid.GetSizeY() );
	
	NAI::CFastRenderer::SResult *pResult = fastRender.resGrid[ nY ][ nX ];

	// Processing the result
	while ( pResult )
	{
		if ( IsValid(pResult->pObject) )
		{
			CDynamicCast<const SVisObjDescBase> pVisObjDescBase = pResult->pObject;
			if ( pVisObjDescBase )
			{
				const int nID = pVisObjDescBase->GetID();

				if ( (nID & 0x80000000) && data[eScene]->attachIDToMapObjID.find( nID ) != data[eScene]->attachIDToMapObjID.end() )
					pickObjects.push_back( data[eScene]->attachIDToMapObjID[nID] );
				else
					pickObjects.push_back( nID );
			}
		}

		pResult = pResult->pNext;
	}
	*/
	return;
}

void CScene::PickObjects( std::list<int> &pickObjects, const CVec2 &vScreenPos1, const CVec2 &vScreenPos2,
												 EPickObjects eRadiusCoeff, const EPickObjectsClass ePickObjsClass )
{
	if ( fastRender.resGrid.IsEmpty() )
		return;

	// Lets find square that lies inside vScreenPos1, vScreenPos2
	CVec2 vScreenRect = NGfx::GetScreenRect();
	CVec2 vPosMin( vScreenPos1.x/vScreenRect.x, vScreenPos1.y/vScreenRect.y );
	CVec2 vPosMax( vScreenPos2.x/vScreenRect.x, vScreenPos2.y/vScreenRect.y );

	if ( vPosMin.x > vPosMax.x )
		std::swap( vPosMin.x, vPosMax.x );

	if ( vPosMin.y > vPosMax.y )
		std::swap( vPosMin.y, vPosMax.y );

	if ( vPosMin.x < 0.0f )
		vPosMin.x = 0.0f;

	if ( vPosMin.y < 0.0f )
		vPosMin.y = 0.0f;

	if ( vPosMax.x > 1.0f )
		vPosMax.x = 1.0f;

	if ( vPosMax.y > 1.0f )
		vPosMax.y = 1.0f;

	if ( vPosMin.x >= vPosMax.x || vPosMin.y >= vPosMax.y )
		return;

	int nMinX = GetGridPosition( vPosMin.x, fastRender.resGrid.GetSizeX() );
	int nMaxX = GetGridPosition( vPosMax.x, fastRender.resGrid.GetSizeX() );
	int nMinY = GetGridPosition( (1.0f-vPosMax.y), fastRender.resGrid.GetSizeY() );
	int nMaxY = GetGridPosition( (1.0f-vPosMin.y), fastRender.resGrid.GetSizeY() );

	// Processing results
	static std::vector<const CObjectBase*> foundObjects;
	foundObjects.resize( 0 );

	for ( int iY = nMinY; iY <= nMaxY; ++iY )
	{
		for ( int iX = nMinX; iX <= nMaxX; ++iX )
		{
			NAI::CFastRenderer::SResult *pResult = fastRender.resGrid[ iY ][ iX ];
			while( pResult )
			{
				if ( IsValid(pResult->pObject) )
				{
					// Check if we already added this object
					if ( find( foundObjects.begin(), foundObjects.end(), pResult->pObject ) == foundObjects.end() )
					{
						// Adding object
						foundObjects.push_back( pResult->pObject );
						CDynamicCast<const SVisObjDescBase> pVisObjDescBase = pResult->pObject;

						if ( pVisObjDescBase )
						{
							const int nID = pVisObjDescBase->GetID();

							if ( (nID & 0x80000000) && data[eScene]->attachIDToMapObjID.find( nID ) != data[eScene]->attachIDToMapObjID.end() )
								pickObjects.push_back( data[eScene]->attachIDToMapObjID[nID] );
							else
								pickObjects.push_back( nID );
						}
					}
				}

				pResult = pResult->pNext;
			}
		}
	}
} 

void CScene::GetObstacleObjects( std::list<int> *pObstacleObjects, const CVec2 &vScreenPos1, const CVec2 &vScreenPos2, const SObjectFilter &canBeCovered, const SObjectFilter &canBeObstacle )
{
	if ( fastRender.resGrid.IsEmpty() )
		return;

	// Lets find square that lies inside vScreenPos1, vScreenPos2
	CVec2 vScreenRect = NGfx::GetScreenRect();
	CVec2 vPosMin( vScreenPos1.x/vScreenRect.x, vScreenPos1.y/vScreenRect.y );
	CVec2 vPosMax( vScreenPos2.x/vScreenRect.x, vScreenPos2.y/vScreenRect.y );

	if ( vPosMin.x > vPosMax.x )
		std::swap( vPosMin.x, vPosMax.x );

	if ( vPosMin.y > vPosMax.y )
		std::swap( vPosMin.y, vPosMax.y );

	if ( vPosMin.x < 0.0f )
		vPosMin.x = 0.0f;

	if ( vPosMin.y < 0.0f )
		vPosMin.y = 0.0f;

	if ( vPosMax.x > 1.0f )
		vPosMax.x = 1.0f;

	if ( vPosMax.y > 1.0f )
		vPosMax.y = 1.0f;

	if ( vPosMin.x >= vPosMax.x || vPosMin.y >= vPosMax.y )
		return;

	int nMinX = GetGridPosition( vPosMin.x, fastRender.resGrid.GetSizeX() );
	int nMaxX = GetGridPosition( vPosMax.x, fastRender.resGrid.GetSizeX() );
	int nMinY = GetGridPosition( (1.0f-vPosMax.y), fastRender.resGrid.GetSizeY() );
	int nMaxY = GetGridPosition( (1.0f-vPosMin.y), fastRender.resGrid.GetSizeY() );

	// Processing results
	std::unordered_set<int> foundObjects;

	for ( int iY = nMinY; iY <= nMaxY; ++iY )
	{
		for ( int iX = nMinX; iX <= nMaxX; ++iX )
		{
			// First, lets find covered far object
			NAI::CFastRenderer::SResult *pCovered = 0;
			for( NAI::CFastRenderer::SResult *pResult = fastRender.resGrid[ iY ][ iX ]; pResult; pResult = pResult->pNext )
			{
				CDynamicCast<const SVisObjDescBase> pVisObjDescBase = pResult->pObject;

				// Check if this object can be covered by others
				if ( pVisObjDescBase && canBeCovered( pVisObjDescBase->GetID() ) )
					pCovered = pResult;					
			}

			if ( !pCovered )
				continue;

			// So we will add all obstacles in this node
			for( NAI::CFastRenderer::SResult *pResult = fastRender.resGrid[ iY ][ iX ]; pResult != pCovered; pResult = pResult->pNext )
			{
				if ( !IsValid(pResult->pObject) )
					continue;

				CDynamicCast<const SVisObjDescBase> pVisObjDescBase = pResult->pObject;

				// Check if this object can obstacle
				if ( !pVisObjDescBase || !canBeObstacle( pVisObjDescBase->GetID() ) )
					continue;

				// Check if we already added this object
				if ( foundObjects.find( pVisObjDescBase->GetID() ) != foundObjects.end() )
					continue;

				// Adding object
				foundObjects.insert( pVisObjDescBase->GetID() );
				pObstacleObjects->push_back( pVisObjDescBase->GetID() );
			}
		}
	}
	return;
}

void CScene::GetCoveredObjects( std::list<int> *pCoveredObjects, const SObjectFilter &canBeCovered, const SObjectFilter &canBeObstacle )
{
	std::unordered_set<int> foundObjects;
	for ( int iY = 0; iY < fastRender.resGrid.GetSizeY(); ++iY )
	{
		for ( int iX = 0; iX < fastRender.resGrid.GetSizeX(); ++iX )
		{
			bool bNodeIsCovered = false;
			for( NAI::CFastRenderer::SResult *pResult = fastRender.resGrid[ iY ][ iX ]; pResult; pResult = pResult->pNext )
			{
				if ( !IsValid(pResult->pObject) )
					continue;

				if ( !bNodeIsCovered )
				{
					CDynamicCast<const SVisObjDescBase> pVisObjDescBase = pResult->pObject;

					// Check if this object can obstacle others
					if ( pVisObjDescBase && canBeObstacle( pVisObjDescBase->GetID() ) )
						bNodeIsCovered = true;					
				}
				else
				{
					CDynamicCast<const SVisObjDescBase> pVisObjDescBase = pResult->pObject;

					// Check if this object can be covered
					if ( !pVisObjDescBase || !canBeCovered( pVisObjDescBase->GetID() ) )
						continue;

					// Check if we already added this object
					if ( foundObjects.find( pVisObjDescBase->GetID() ) != foundObjects.end() )
						continue;

					// Adding object
					foundObjects.insert( pVisObjDescBase->GetID() );
					pCoveredObjects->push_back( pVisObjDescBase->GetID() );
				}
			}
		}
	}
	return;
/*
	CVec3 vCameraPos = Camera()->GetPos();

	data[eScene]->pAIMapVisitor->Sync();

	for ( list<int>::const_iterator it = objects.begin(); it != objects.end(); ++it )
	{
		SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( *it );
		if ( pos == data[eScene]->visObjects.end() ) 
			continue;
		SVisObjDescBase *pObj = pos->second;

		SFBTransform place = pObj->GetPlacement();
		CVec3 vObjPos;
		vObjPos.x = place.forward._14;
		vObjPos.y = place.forward._24;
		vObjPos.z = place.forward._34;

		CRay ray;
		ray.ptOrigin = vCameraPos;
		ray.ptDir = vObjPos - vCameraPos;

		// ищем все объекты, закрывающие данный
		vector<NAI::SInterval> intervals;
		data[eScene]->pAIMap->Trace( ray, &intervals, 1 );
		list<int> obstacles;
		for ( vector<NAI::SInterval>::const_iterator it = intervals.begin(); it != intervals.end(); ++it )
		{
			const SVisObjDescBase *pVisObjDesc = dynamic_cast_ptr<const SVisObjDescBase*>(it->pSrc->pUserData);
			if ( !pVisObjDesc )
					continue;
			int id = pVisObjDesc->GetID();
			if ( id == pObj->GetID() )
			{
				if ( !obstacles.empty() )
					obstacles.push_back( id );
				break;
			}
			obstacles.push_back( id );
		}
		pObstacles->push_back( list<int>() );
		pObstacles->back().swap( obstacles );
	}
*/
}


