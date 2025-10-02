#pragma once


//
//		specific part, required by terragen
//

namespace NDb
{
	struct STerrainAIProperties;
	struct SVSOInstance;
}

interface ITerraAIObserver : public CObjectBase
{
	virtual void UpdateHeights( const int nX1, const int nY1, const int nX2, const int nY2, const CArray2D<float> &heights ) = 0;
	//
	virtual void SetTerraTypes( const vector<NDb::STerrainAIProperties> &params ) = 0;
	virtual void UpdateTypes( const int nX1, const int nY1, const int nX2, const int nY2, const CArray2D<BYTE> &types ) = 0;
	virtual void AddRoad( const NDb::SVSOInstance *pInstance ) = 0;
	virtual void AddRiver( const NDb::SVSOInstance *pInstance ) = 0;
	virtual void AddCrag( const NDb::SVSOInstance *pInstance ) = 0;
	virtual void AddWaterLine( const NDb::SVSOInstance *pInstance, const bool bIsLake ) = 0;
	//
	virtual float GetZ( float x, float y ) const = 0;
	virtual float GetTileHeight( int nX, int nY ) const = 0;
	virtual void UpdateZ( CVec3 *pvPos ) = 0;
	virtual DWORD GetNormal( const CVec2 &vPoint ) const = 0;
	virtual bool GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const = 0;
	virtual bool GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const = 0;
	virtual void InitHeights4Editor( int nSizeX, int nSizeY ) = 0;
	//
	virtual void FinalizeUpdates() = 0;
	//
	virtual void ToggleShowPassability() { }
	virtual void DrawPassabilities() const = 0;
	virtual void SetPassMarkers( const int color, const int aiClass, const int freeClass, const int nBoundTileRadius ) { }
	virtual void DumpMaxes( const string &szFileName, const int aiClass ) { }
	virtual bool IsPassabilityOn() const { return false; }
	virtual bool IsBridge( const int nX, const int nY ) const { return false; }
};


