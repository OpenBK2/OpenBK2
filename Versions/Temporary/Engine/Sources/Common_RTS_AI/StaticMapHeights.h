#pragma once

#include "Common_RTS_AI_export.h"


#include "..\Misc\Spline.h"
#include "..\Misc\2DArray.h"

class COMMON_RTS_AI_EXPORT CStaticMapHeights : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CStaticMapHeights );
	
	struct SOldHeights
	{
		int nX1, nY1;
		CArray2D<float> heights;

		SOldHeights() : nX1( -1 ), nY1( -1 ) {}
		SOldHeights( const int _nX1, const int _nY1, const CArray2D<float> &_heights ) :
			nX1( _nX1 ), nY1( _nY1 ), heights( _heights ) {}
	};
	typedef hash_map< int, SOldHeights > CHeightsMap;

	// высоты по узлам визуальной сетки
	CArray2D<float> heights;
	CBetaSpline3D betaSpline3D;
	// высоты по центрам AI тайлов
	CArray2D<float> tileHeights;
	int nStaticMapSizeX;
	int nStaticMapSizeY;
	CHeightsMap oldHeightsMap;
	int nLastHeightsID;
	int nTileSize;
	void GetPoint4Spline( const CVec2 &vPoint, float *pu, float *pv, float ptCtrls[] ) const;
	void Init( const int nSizeXInTiles, const int nSizeYInTiles, const int nTileSize );
	const float GetHeight( const int x, const int y ) const;

	int operator&( IBinSaver &f ) { NI_ASSERT( false, "Can't serialize CStaticMapHeights, should be restored after load" ); return 0; }
public:
	CStaticMapHeights() : nStaticMapSizeX( 0 ), nStaticMapSizeY( 0 ), nTileSize( 0 ), nLastHeightsID ( -1 ) { }

	CStaticMapHeights( const int nSizeXInTiles, const int nSizeYInTiles, const int nTileSize );
	void Init4Editor( const int nSizeXInTiles, const int nSizeYInTiles, const int nTileSize );

	//void LoadNormals( const struct ::STerrainAIInfo *pTerrainAIInfo );
	// init by plane with 0 height
	// map size - in AI TILES
	
	// update given range of height
	// для UpdateHeights массив "heghts" должен быть размером с карту
	void UpdateHeights( const int nX1, const int nY1, const int nX2, const int nY2, const CArray2D<float> &heghts );
	void FinalizeUpdateHeights( const int nX1, const int nY1, const int nX2, const int nY2 );
	void FinalizeUpdateHeights();

	// для UpdateLocalHeights массив "heghts" задает высоты только для фрагмента и в координатах AI
	// НЕТ ОБНОВЛЕНИЯ КРАЯ КАРТЫ ВЫСОТ, возвращает ID, который необходимо использовать в RestoreHeights;
	// int UpdateLocalHeights( const int nX1, const int nY1, const CArray2D<float> &heghts );
	int UpdateLocalHeights( const int nX1, const int nY1, const CArray2D<bool> &bridge, const float fBridgeHeight );
	void RestoreHeights( const int nID );
	bool GetLocalHeightsInfo( SVector *pvTopLeft, SVector *pvBottomRight, const int nID ) const;

	const bool GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const;
	const bool GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const;


	const float GetVisZ( float x, float y ) const;
	const void  UpdateVisZ( CVec3 *pVec ) const;
	inline const float GetZ( float x, float y ) const { return GetVisZ( x, y ); }
	inline const float GetZ( const CVec2 &vPos ) const { return GetVisZ( vPos.x, vPos.y ); }
	inline const void  UpdateZ( CVec3 *pVec ) const { UpdateVisZ( pVec ); }
	const DWORD GetNormal( const CVec2 &vPoint ) const { return GetNormal( vPoint.x, vPoint.y ); }
	const DWORD GetNormal( const float x, const float y ) const;
	const float GetTileHeight( const int nTileX, const int nTileY ) const;
	//const float GetVisHeight( const int nVisTileX, const int nVisTileY ) const;
	void SetHeightForPatternApplying( const int nX, const int nY, const float fHeight );

	const CVec3 Get3DPoint( const CVec2 &vPoint ) const { return CVec3( vPoint, GetZ( vPoint ) ); }
	const CVec3 GetGroundPoint( const CVec3 &vPoint ) { return CVec3( vPoint.x, vPoint.y, GetZ( vPoint.x, vPoint.y ) ); }
	const float GetZ( const CVec3 &vPoint ) const { return GetZ( vPoint.x, vPoint.y ); }
};

