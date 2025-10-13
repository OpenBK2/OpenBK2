#pragma once

#include "Common_RTS_AI_export.h"

#include "AIClasses.h"

#include <cstdint>

class CStaticMapHeights;
class CTerrain;
class CArray2D1Bit;
struct SObjTileInfo;

template <int N> struct SGenericNumber
{
	int operator()() const { return N; }
};

class COMMON_RTS_AI_EXPORT CAIMap : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CAIMap );
	
	ZDATA
		int nSizeX, nSizeY;
		int nTileSize;
		int nMaxUnitTileRadius;
		int nMaxMapSize;
    ZONSERIALIZE
	ZEND public: int operator&( IBinSaver &f ) { f.Add(2,&nSizeX); f.Add(3,&nSizeY); f.Add(4,&nTileSize); f.Add(5,&nMaxUnitTileRadius); f.Add(6,&nMaxMapSize); OnSerialize( f ); return 0; } private:
	// don't serialize, restore after load
	CObj<CStaticMapHeights> pHeights;
	CObj<CTerrain> pTerrain;

	template <class TContainter, int N>
		inline bool ProcessLargeCircleTiles( const CVec2 &vCenter, const float fRadius, TContainter *pTiles, const EAIClasses aiClass, const SGenericNumber<N>& );
	template <class TContainter, int N>
		inline bool ProcessCircleTiles( const CVec2 &vCenter, const float fRadius, TContainter *pTiles, const EAIClasses aiClass, const SGenericNumber<N>& );
	template <class TContainter, int N>
		inline bool ProcessQuadrangleTiles( const CVec2 &v1, const CVec2 &v2, const CVec2 &v3, const CVec2 &v4, TContainter *pTiles, const EAIClasses aiClass, const SGenericNumber<N>& );
	template <class TContainter>
		inline void PushTile( const SVector &vCenter, const SVector &tile, TContainter *tiles, CArray2D1Bit &mask, const int nMaxRadius );
	template <class TContainter>
		inline bool Process8Tiles( const SVector &vCenter, const SVector &vOffset, TContainter *pTiles, const EAIClasses aiClass, const bool bAddOnly, CArray2D1Bit &mask );
	template <class TContainter>
		inline bool ProcessLineTiles( const SVector &vCenter, const SVector &vOffset, TContainter *pTiles, const EAIClasses aiClass, const bool bAddOnly, CArray2D1Bit &mask );

	std::vector<SVector> &GetTilesForCircle( const float fRadius );
	void AddTile( const SVector &tile, std::vector<SVector> &tiles, CArray2D1Bit &mask, const int nMaxRadius );
	void Add8TilesEven( const SVector vOffset, std::vector<SVector> &tiles, CArray2D1Bit &mask, const int nMaxRadius );
	void AddLinesEven( const SVector vOffset, std::vector<SVector> &tiles, CArray2D1Bit &mask, const int nMaxRadius );
	void Add8TilesOdd( const SVector vOffset, std::vector<SVector> &tiles, CArray2D1Bit &mask, const int nMaxRadius );
	void AddLinesOdd( const SVector vOffset, std::vector<SVector> &tiles, CArray2D1Bit &mask, const int nMaxRadius );
	SVector GetOffset( const uint16_t wAngle, const int nDiameter );
	bool IsLocked( const int x, const int y, const EAIClasses aiClass ) const;
	bool IsLocked( const SVector &tile, const EAIClasses aiClass ) const;

	void OnSerialize( IBinSaver &saver );

public:
	CAIMap() : nMaxMapSize( 0 ) { }
	CAIMap( const int nSizeX, const int nSizeY, const int nTileSize, const int nMaxUnitTileRadius, const int nMaxMapSize );

	CStaticMapHeights* GetHeights() const { return pHeights; }
	CTerrain* GetTerrain() const { return pTerrain; }

	const int GetSizeX() const { return nSizeX; }
	const int GetSizeY() const { return nSizeY; }
	const int GetTileSize() const { return nTileSize; }
	const int GetMaxMapSize() const { return nMaxMapSize; }

	bool IsTileInside( const int x, const int y ) const;
	bool IsTileInside( const SVector &tile ) const;
	bool IsPointInside( const float x, const float y ) const;
	bool IsPointInside( const CVec2 &point ) const;
	bool IsRectInside( const SRect &rect ) const;

	const int GetMaxUnitTileRadius() const { return nMaxUnitTileRadius; }
	const SVector GetTile( const float x, const float y ) const;
	const SVector GetTile( const CVec2 &point ) const;
	CVec2 GetPointByTile( const int x, const int y ) const;
	CVec2 GetPointByTile( const SVector &tile ) const;
	CVec2 GetPointByTile( const SObjTileInfo &tileInfo ) const;
	const CVec2 GetCenterOfTile( const float x, const float y ) const;
	const CVec2 GetCenterOfTile( const CVec2 &point ) const;

	//
	void RecreateCircles();
	bool IsRectOnLockedTiles( const SRect &rect, const EAIClasses aiClass );
	bool IsCircleOnLockedTiles( const CCircle &circle, const EAIClasses aiClass );

	// возвращает tiles, которые накрывает данный четырёхугольник
	template<class TContainter>
		inline void GetTilesCoveredByQuadrangle( const CVec2 &v1, const CVec2 &v2, const CVec2 &v3, const CVec2 &v4, TContainter *pTiles );
	// возвращает tiles, которые накрывает данный rect
	template<class TContainter>
		inline void GetTilesCoveredByRect( const SRect &rect, TContainter *pTiles );
	template<class TContainter>
		inline void GetTilesCoveredByCircle( const CVec2 &vCenter, const float fRadius, TContainter *pTiles );
	template<class TContainter>
		inline void GetTilesCoveredByLargeCircle( const CVec2 &vCenter, const float fRadius, TContainter *pTiles );
	//возвращает tiles, которые пересекаются со сторонами данного rect
	template<class TContainter>
		inline void GetTilesCoveredByRectSides( const SRect &rect, TContainter *pTiles );

	void Clear();
};

#include "AIMap.hpp"

