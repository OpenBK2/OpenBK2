#if !defined(__COMMON_TOOLS__SCENE_DRAW__)
#define __COMMON_TOOLS__SCENE_DRAW__
#pragma once

#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../stats_b2_m1/iconsset.h"
#include "Tools_SceneGeometry.h"
#include "../Image/ImageColor.h"
#include "EditorScene.h"
#include "../SceneB2/Terrain.h"
#include "../stats_b2_m1/Vis2AI.h"

namespace NDb
{
	struct SModel;
}

class CSceneDrawTool
{
	static const float SCENE_Z_SHIFT;

	// CRAP{ HASH_SET
	typedef hash_map<int, DWORD> CModelIDSet;
	typedef hash_map<int, DWORD> CPolylineIDSet;
	// CRAP} HASH_SET
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SModelInfo
	{
		NDb::SModel *pModel;
		CVec3 vPos;
		CQuat qRot;
		CVec3 vScale;
		//		
		SModelInfo() : pModel( 0 ), vPos( VNULL3 ), qRot( QNULL ), vScale( VNULL3 ) {}
		SModelInfo( const SModelInfo &rModelInfo )
			: pModel( rModelInfo.pModel ),
				vPos( rModelInfo.vPos ),
				qRot( rModelInfo.qRot ),
				vScale( rModelInfo.vScale ) {}
		SModelInfo& operator=( const SModelInfo &rModelInfo )
		{
			if( &rModelInfo != this )
			{
				pModel = rModelInfo.pModel;
				vPos = rModelInfo.vPos;
				qRot = rModelInfo.qRot;
				vScale = rModelInfo.vScale;
			}
			return *this;
		}	
	};
	typedef list<SModelInfo> CModelInfoList;
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SPolylineInfo
	{
		vector<CVec3> points;
		CVec3 vColor;
		bool bDepthCheck;
		//		
		SPolylineInfo() : vColor( VNULL3 ), bDepthCheck( true ) {}
		SPolylineInfo( const SPolylineInfo &rPolylineInfo )
			: points( rPolylineInfo.points ),
				vColor( rPolylineInfo.vColor ),
				bDepthCheck( rPolylineInfo.bDepthCheck ) {}
		SPolylineInfo& operator=( const SPolylineInfo &rPolylineInfo )
		{
			if( &rPolylineInfo != this )
			{
				points = rPolylineInfo.points;
				vColor = rPolylineInfo.vColor;
				bDepthCheck = rPolylineInfo.bDepthCheck;
			}
			return *this;
		}	
	};
	typedef list<SPolylineInfo> CPolylineInfoList;
	
	CModelIDSet modelIDSet;
	CPolylineIDSet polylineIDSet;
	//
	CModelInfoList modelInfoList;
	CPolylineInfoList polylineInfoList;

public:
	CSceneDrawTool();
	~CSceneDrawTool();
	//
	void Draw();
	void Clear();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<class TPoint>
	void DrawLine( const TPoint &rBegin, const TPoint &rEnd,  DWORD dwColor, bool bDepthCheck )
	{
		const CVec3 vBegin = GetPointType( rBegin, static_cast<CVec3*>( 0 ) );
		const CVec3 vEnd = GetPointType( rEnd, static_cast<CVec3*>( 0 ) );
		CPolylineInfoList::iterator posNewPolyline = polylineInfoList.insert( polylineInfoList.end(), SPolylineInfo() );
		{
			posNewPolyline->points.push_back( vBegin );
			posNewPolyline->points.push_back( vEnd );
			//
			GetVec3FromARGBColor( &( posNewPolyline->vColor ), dwColor );
			posNewPolyline->bDepthCheck = bDepthCheck;
		}
	}
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<class TPoint>
	void Draw3DLine( const TPoint &rBegin, const TPoint &rEnd,  DWORD dwColor, bool bDepthCheck, int nSegmentCount )
	{
		CPolylineInfoList::iterator posNewPolyline = polylineInfoList.insert( polylineInfoList.end(), SPolylineInfo() );
		//
		CVec3 vBegin = GetPointType( rBegin, static_cast<CVec3*>( 0 ) );
		CVec3 vEnd = GetPointType( rEnd, static_cast<CVec3*>( 0 ) );
		if ( nSegmentCount > 1 )
		{
			const CVec3 vPart = ( vEnd - vBegin ) / nSegmentCount;
			for ( int nSegmentIndex = 0; nSegmentIndex <= nSegmentCount; ++nSegmentIndex )
			{
				CVec3 vPoint = vBegin + vPart * nSegmentIndex;
				vPoint.z = GetTerrainHeight( vPoint.x, vPoint.y ) + ( bDepthCheck ? SCENE_Z_SHIFT : 0.0f );
				posNewPolyline->points.push_back( vPoint );
			}
		}
		else
		{
			vBegin.z = GetTerrainHeight( vBegin.x, vBegin.y ) + ( bDepthCheck ? SCENE_Z_SHIFT : 0.0f );
			vEnd.z = GetTerrainHeight( vEnd.x, vEnd.y ) + ( bDepthCheck ? SCENE_Z_SHIFT : 0.0f );
			posNewPolyline->points.push_back( vBegin );
			posNewPolyline->points.push_back( vEnd );
		}
		//
		GetVec3FromARGBColor( &( posNewPolyline->vColor ), dwColor );
		posNewPolyline->bDepthCheck = bDepthCheck;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Draw3DLine( const CTPoint<int> &rBegin, const CTPoint<int> &rEnd,  DWORD dwColor, bool bGridHeights, bool bConnectors, bool bDepthCheck )
	{
		ITerraManager *pTerraManager = EditorScene()->GetTerraManager();
		if ( ( pTerraManager != 0 ) && ( ( rBegin.x == rEnd.x ) || ( rBegin.y == rEnd.y ) ) )
		{
			CPolylineInfoList::iterator posNewPolyline = polylineInfoList.insert( polylineInfoList.end(), SPolylineInfo() );
			if ( bConnectors && bGridHeights )
			{
				CVec3 vPoint = CVec3( rBegin.x * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE,
															rBegin.y * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE,
															0.0f );
				vPoint.z = GetTerrainHeight( vPoint.x, vPoint.y ) + ( bDepthCheck ? SCENE_Z_SHIFT : 0.0f );
				posNewPolyline->points.push_back( vPoint );
			}
			if ( rBegin.x == rEnd.x )
			{
				for ( int nPointIndex = rBegin.y; nPointIndex <= rEnd.y; ++nPointIndex )
				{
					CVec3 vPoint = CVec3( rBegin.x * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE,
																nPointIndex * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE,
																0.0f );
					if ( bGridHeights )
					{
						vPoint.z = pTerraManager->GetTerraHeightFast( rBegin.x, nPointIndex ) * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE / VIS_TILE_SIZE + ( bDepthCheck ? SCENE_Z_SHIFT : 0.0f );
					}
					else
					{
						vPoint.z = GetTerrainHeight( rBegin.x * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE, nPointIndex * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE ) + ( bDepthCheck ? SCENE_Z_SHIFT : 0.0f );
					}
					posNewPolyline->points.push_back( vPoint );
				}
			}
			else if ( rBegin.y == rEnd.y )
			{
				for ( int nPointIndex = rBegin.x; nPointIndex <= rEnd.x; ++nPointIndex )
				{
					CVec3 vPoint = CVec3( nPointIndex * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE,
																rBegin.y * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE,
																0.0f );
					if ( bGridHeights )
					{
						vPoint.z = pTerraManager->GetTerraHeightFast( nPointIndex, rBegin.y ) * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE / VIS_TILE_SIZE + ( bDepthCheck ? SCENE_Z_SHIFT : 0.0f );
					}
					else
					{
						vPoint.z = GetTerrainHeight( nPointIndex * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE, rBegin.y * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE ) + ( bDepthCheck ? SCENE_Z_SHIFT : 0.0f );
					}
					posNewPolyline->points.push_back( vPoint );
				}
			}
			if ( bConnectors && bGridHeights )
			{
				CVec3 vPoint = CVec3( rEnd.x * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE,
															rEnd.y * AI_TILE_SIZE * AI_TILES_IN_VIS_TILE,
															0.0f );
				vPoint.z = GetTerrainHeight( vPoint.x, vPoint.y ) + ( bDepthCheck ? SCENE_Z_SHIFT : 0.0f );
				posNewPolyline->points.push_back( vPoint );
			}
			GetVec3FromARGBColor( &( posNewPolyline->vColor ), dwColor );
			posNewPolyline->bDepthCheck = bDepthCheck;
		}
	}
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<class TPointList>
	void DrawPolyline( const TPointList &rPolintList, DWORD dwColor, bool bClosed, bool bDepthCheck )
	{
		TPointList::const_iterator itTestPoint = rPolintList.begin();
		// нет точек
		if ( itTestPoint == rPolintList.end() )
		{
			return; 
		}
		++itTestPoint;
		// одна точка
		if ( itTestPoint == rPolintList.end() )
		{
			return;
		}
		//
		CPolylineInfoList::iterator posNewPolyline = polylineInfoList.insert( polylineInfoList.end(), SPolylineInfo() );
		for ( TPointList::const_iterator itPoint = rPolintList.begin(); itPoint != rPolintList.end(); ++itPoint )
		{
			const CVec3 vPoint = GetPointType( *itPoint, static_cast<CVec3*>( 0 ) );
			posNewPolyline->points.push_back( vPoint );
		}
		if ( bClosed )
		{
			const CVec3 vPoint = GetPointType( *( rPolintList.begin() ), static_cast<CVec3*>( 0 ) );
			posNewPolyline->points.push_back( vPoint );
		}
		GetVec3FromARGBColor( &( posNewPolyline->vColor ), dwColor );
		posNewPolyline->bDepthCheck = bDepthCheck;
	}
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<class TPointList>
	void Draw3DPolyline( const TPointList &rPolintList, DWORD dwColor, bool bClosed, bool bDepthCheck )
	{
		TPointList::const_iterator itTestPoint = rPolintList.begin();
		// нет точек
		if ( itTestPoint == rPolintList.end() )
		{
			return; 
		}
		++itTestPoint;
		// одна точка
		if ( itTestPoint == rPolintList.end() )
		{
			return;
		}
		//
		CPolylineInfoList::iterator posNewPolyline = polylineInfoList.insert( polylineInfoList.end(), SPolylineInfo() );
		for ( TPointList::const_iterator itPoint = rPolintList.begin(); itPoint != rPolintList.end(); ++itPoint )
		{
			CVec3 vPoint = GetPointType( *itPoint, static_cast<CVec3*>( 0 ) );
			vPoint.z = GetTerrainHeight( vPoint.x, vPoint.y ) + ( bDepthCheck ? SCENE_Z_SHIFT : 0.0f );
			posNewPolyline->points.push_back( vPoint );
		}
		if ( bClosed )
		{
			CVec3 vPoint = GetPointType( *( rPolintList.begin() ), static_cast<CVec3*>( 0 ) );
			vPoint.z = GetTerrainHeight( vPoint.x, vPoint.y ) + ( bDepthCheck ? SCENE_Z_SHIFT : 0.0f );
			posNewPolyline->points.push_back( vPoint );
		}
		GetVec3FromARGBColor( &( posNewPolyline->vColor ), dwColor );
		posNewPolyline->bDepthCheck = bDepthCheck;
	}
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<class TPoint>
	void DrawCircle( const TPoint &rCenter, float fRadius, int nParts, DWORD dwColor, bool bDepthCheck )
	{	
		if ( nParts < 2 )
		{
			return;
		}
		CVec3 vCenter = GetPointType( rCenter, static_cast<CVec3*>( 0 ) );
		CPolylineInfoList::iterator posNewPolyline = polylineInfoList.insert( polylineInfoList.end(), SPolylineInfo() );
		for ( int nIndex = 0; nIndex <= nParts; ++nIndex )
		{
			float alpha = FP_2PI * nIndex / nParts;
			const CVec3 vPoint = CVec3( fRadius * cos( alpha ) + vCenter.x,
																	fRadius * sin( alpha ) + vCenter.y,
																	vCenter.z );
			posNewPolyline->points.push_back( vPoint );
		}
		GetVec3FromARGBColor( &( posNewPolyline->vColor ), dwColor );
		posNewPolyline->bDepthCheck = bDepthCheck;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<class TPoint>
	void Draw3DCircle( const TPoint &rCenter, float fRadius, int nParts, DWORD dwColor, bool bDepthCheck )
	{	
		if ( nParts < 2 )
		{
			return;
		}
		CVec3 vCenter = GetPointType( rCenter, static_cast<CVec3*>( 0 ) );
		CPolylineInfoList::iterator posNewPolyline = polylineInfoList.insert( polylineInfoList.end(), SPolylineInfo() );
		for ( int nIndex = 0; nIndex <= nParts; ++nIndex )
		{
			float alpha = FP_2PI * nIndex / nParts;
			CVec3 vPoint = CVec3( fRadius * cos( alpha ) + vCenter.x,
														fRadius * sin( alpha ) + vCenter.y,
														vCenter.z );
			vPoint.z = GetTerrainHeight( vPoint.x, vPoint.y ) + ( bDepthCheck ? SCENE_Z_SHIFT : 0.0f );
			posNewPolyline->points.push_back( vPoint );
		}
		GetVec3FromARGBColor( &( posNewPolyline->vColor ), dwColor );
		posNewPolyline->bDepthCheck = bDepthCheck;
	}
};

#endif // !defined(__COMMON_TOOLS__SCENE_DRAW__)

