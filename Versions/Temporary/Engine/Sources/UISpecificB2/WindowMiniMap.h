#pragma once

#include "UI/Window.h"
#include "UISpecificB2/DBUISpecificB2.h"
#include "3Dmotor/GfxBuffers.h"
#include "System/Dg.h"
#include "Misc/BitData.h"
#include "UISpecificB2.h"

#include <cstdint>

class CMiniMapLayer :  public CPtrFuncBase< NGfx::CTexture >
{
	OBJECT_BASIC_METHODS(CMiniMapLayer);
	CArray2D<NGfx::SPixel8888> points;
	int nWidth, nHeight;
	bool bNeedUpdate;
	int nLastUpdateTime;
protected:
	bool NeedUpdate();
	void Recalc();

public:
	CMiniMapLayer() : nWidth( 0 ), nHeight( 0 ), bNeedUpdate( false ), nLastUpdateTime( 0 ) {}

	void SetSize( const int nWidth, const int nHeight );
	int GetWidth() const { return nWidth; }
	int GetHeight() const { return nHeight; }
	void PutPixel( const int x, const int y, const NGfx::SPixel8888 &color );
	const NGfx::SPixel8888 GetPixel( const int x, const int y );
	void SetNeedUpdate() { bNeedUpdate = true; }
	void SetNeedUpdateNow() { bNeedUpdate = true; nLastUpdateTime = 0; }
	void Clear();

	int operator&( IBinSaver &saver );
};

class CDrawMiniMapPixelFunctional
{
	CPtr<CMiniMapLayer> pMiniMapLayer;
	CArray2D1Bit *pMaskMiniMap;
	NGfx::SPixel8888 color;
	int nWidth, nHeight;
public:
	CDrawMiniMapPixelFunctional( CMiniMapLayer *_pMiniMapLayer, NGfx::SPixel8888 _color, CArray2D1Bit *_pMaskMiniMap )
		: pMiniMapLayer( _pMiniMapLayer ), color( _color ), pMaskMiniMap( _pMaskMiniMap )
	{
		nWidth = pMiniMapLayer->GetWidth();
		nHeight = pMiniMapLayer->GetHeight();
	}

	void operator()( int x, int y )
	{
		if ( x >= 0 && y >= 0 && x < nWidth && y < nHeight && pMaskMiniMap->GetData( x, y ) )
			pMiniMapLayer->PutPixel( x, y, color );
	}
};

// Minimap returns game's evets minimap_up, minimap_down and minimap_move. where first parameter
// is cursor position in map's AI coordinate from. Minimap only send's messages when some button
// pressed on it, it doesn't generate message when mouse move over it.

class CWindowMiniMap : public CWindow, public IMiniMap
{
	OBJECT_BASIC_METHODS(CWindowMiniMap);

	struct STransform
	{
		ZDATA
		float fSin;
		float fCos;
		CVec2 vSize;
		CVec2 vCenter;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&fSin); f.Add(3,&fCos); f.Add(4,&vSize); f.Add(5,&vCenter); return 0; }
		
		STransform()
		{
		}

		void Init( float fAngle, const CVec2 &_vSize, const CVec2 &_vCenter )
		{
			vSize = _vSize;
			vCenter = _vCenter;
			
			fSin = sin( fAngle );
			fCos = cos( fAngle );
		}
		
		CVec2 Transform( const CVec2 &_vPos ) const
		{
			CVec2 vPos( _vPos.x - 0.5f, _vPos.y - 0.5f );
			return CVec2( (fCos * vPos.x + fSin * vPos.y) * vSize.x, 
				-(-fSin * vPos.x + fCos * vPos.y) * vSize.y ) + vCenter;
		}
		CVec2 RotateBackgroundPos( const CVec2 &vPos ) const
		{
			return CVec2( (fCos * vPos.x - fSin * vPos.y), 
				(fSin * vPos.x + fCos * vPos.y) );
		}
		float TransformRadius( float fRadius ) const
		{
			return fRadius * (vSize.x + vSize.y) * 0.5f;
		}
		CVec2 TransformBack( const CVec2 &_vPos ) const
		{
			CVec2 vPos( _vPos - vCenter );
			vPos.x /= vSize.x;
			vPos.y /= -vSize.y;
			return CVec2( fCos * vPos.x - fSin * vPos.y + 0.5f, fSin * vPos.x + fCos * vPos.y + 0.5f );
		}
	};

protected:
	CDBPtr<NDb::SWindowMiniMapShared> pShared;

	CPtr<NDb::SWindowMiniMap> pInstance;

	//mScreenToMap1 for top-left triangle, mScreenToMap2 for bottom-right - matrix for ScreenToAI
	//conversion;
	float mScreenToMap1[2][3], mScreenToMap2[2][3];
	float mMapToScreen1[2][3], mMapToScreen2[2][3];

	CObj< CMiniMapLayer > pUnitsLayer;
	CObj< CMiniMapLayer > pWarFogLayer;
	CObj< CMiniMapLayer > pViewPortLayer;
	CObj< CMiniMapLayer > pFiguresLayer;
	CObj< CMiniMapLayer > pRangesLayer;

	bool bRangesPresent;

	std::vector<int> bButtonDown;
	float fAspect;

	float fMapWidth, fMapHeight;

	std::vector<NGfx::SPixel8888> playerColors;
	std::vector<NGfx::SPixel8888> warFogColors;
	NGfx::SPixel8888 frameColor;
	CArray2D1Bit maskMiniMap;
	CDBPtr< NDb::SMaterial > pMaterial;
	std::vector<SMarker> markers;

	bool bRotable;
	CVec2 vRotableCenter;
	CVec2 vRotableSize;
	STransform rotableTransform;
	CArray2D<uint8_t> warFog;
	//CVec2 vPrevRotableDir;
	const CArray2D<uint8_t> *pWarFog;

	float fAdditionalScale;
	float fPrevRotableAngle;

	CArray2D< std::pair<CVec2, bool> > screenToMap;
	bool bNeedScreenToMapRefresh;
	CVec2 vOldDir;
	
	CDBPtr<NDb::STexture> pTexture;
	
	CVec2 vLastMousePos;
	CVec2 vSize;	// for caching (optimisation)
	
	CDBPtr<NDb::STexture> pRotableBackgroundTexture;
	CVec2 vRotableBackgroundSize;
	CDBPtr<NDb::STexture> pRotableNorthPointTexture;
	CVec2 vRotableNorhtPointSize;
	CDBPtr<NDb::STexture> pRotableForegroundTexture;
private:
	CVec2 GetCWindowSize() const;
	bool CalculateScreenToMapSlow( const CVec2 &vPos, CVec2 *pvRes );

	// преобразовать координаты экрана (курсора мыши) в координаты 0..AISize.x x 0 .. AISize.y
	bool ScreenToMap( const CVec2 &vScreenPos, CVec2 *pvRes );
	// преобразовать координаты 0..1 в координаты minimap'а, т.е. 0..MiniMap.Width x 0..MiniMap.Height
	bool MapToScreen( const CVec2 &vMapPos, CVec2 *pvRes ) const;
	void RadiusMapToScreen( const CVec2 &vMapPos, CVec2 *pvRes );
	float RadiusMapToScreen( float fRadius );
	void MakeTransformParams( const CVec2 &vPoint00, const CVec2 &vPoint01, 
		const CVec2 &vPoint10, const CVec2 &vPoint11 );
	void MakeRotableTransformParams( const CVec2 &vDirView );
	void MakeMaskMimimap();
	void UpdateWarFog();
	void SetBaseRotableParams();
protected:
	virtual NDb::SWindow* GetInstance() { return pInstance; }

	bool MsgMouseMoveEmit( const SGameMessage &msg );

	//{ CWindow
	bool IsActiveArea( const CVec2 &vPos ) const;
	//}
public:
	CWindowMiniMap();
	void LoadMap( const int nWidth, const int nHeight, const int nWarFogLevel );
	void SetLoadingMapParams( int nWidth, int nHeight );
	CVec2 GetAIToScreen( const CVec2 &vPos ) const;

	void InitByDesc( const struct NDb::SUIDesc *pDesc );

	void Visit( struct IUIVisitor *pVisitor );

	bool OnButtonDown( const CVec2 &vPos, const int nButton );
	bool OnButtonUp( const CVec2 &vPos, const int nButton );
	bool OnMouseMove( const CVec2 &vPos, const int nButton );

	void SetUnits( const std::vector< SMiniMapUnitInfo > &vUnits );
	void SetViewport( const std::vector< CVec2 > &vPoints );
	void SetWarFog( const CArray2D<uint8_t> *pWarFogInfo );
	void SetMarkers( const std::vector<SMarker> &markers );
	void SetFigures( const std::vector<SFigure> &figures );

	void SetPlayerColor( const int nPlayer, const NGfx::SPixel8888 &color );

	virtual void SetRangeInfo( const int nUnitID, const SShootAreas &rangeInfo );
	virtual void RemoveAllRangeInfo();

	virtual void SetMaterial( CDBPtr< NDb::SMaterial > pMaterial );
	void SetTexture( const NDb::STexture *pTexture );

	void SetAdditionalScale( float fAdditionalScale );
	virtual void AfterLoad();
	void SetNortDirectionTexture( const NDb::STexture *pTexture );
	int operator&( IBinSaver &saver );
};


