#pragma once

#include "../ui/commandparam.h"
#include "../ui/dbuserinterface.h"
#include "../ui/ui.h"
#include "../input/gamemessage.h"
#include "../ui/uifactory.h"
#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../UI/Window.h"
#include "DBUISpecificB2.h"
#include "../3DMotor/GfxBuffers.h"
#include "../System/DG.h"
#include "../Main/GameTimer.h"
#include "UISpecificB2.h"
#include "../Misc/BitData.h"

class CWindowChapterMapLayer :  public CPtrFuncBase< NGfx::CTexture >
{
	OBJECT_BASIC_METHODS(CWindowChapterMapLayer);

	CArray2D<NGfx::SPixel8888> points;
	int nWidth, nHeight;
	bool bNeedUpdate;
	int nLastUpdateTime;
protected:
	bool NeedUpdate() { 
		int nTime = Singleton<IGameTimer>()->GetAbsTime();
		if ( bNeedUpdate /*&& ( nTime - nLastUpdateTime - 100 ) > 0*/ )
		{
			nLastUpdateTime = nTime;
			return true; 
		}
		else
			return false;
	}
	void Recalc();

public:
	CWindowChapterMapLayer() : nLastUpdateTime( 0 ), bNeedUpdate( false ) { };
	void SetSize( const int nWidth, const int nHeight );
	int GetWidth() const { return nWidth; }
	int GetHeight() const { return nHeight; }
	void PutPixel( const int x, const int y, const NGfx::SPixel8888 &color );
	void SetPixelAlpha( const int x, const int y, const BYTE &nAlpha );
	BYTE GetPixelAlpha( const int x, const int y );
	const NGfx::SPixel8888 GetPixel( const int x, const int y );
	void SetNeedUpdate() { bNeedUpdate = true; }
	void SetNeedUpdateNow() { bNeedUpdate = true; nLastUpdateTime = 0; }
	void Clear();

	int operator&( IBinSaver &saver );
};

class CDrawMapPixelFunctional
{
	CPtr<CWindowChapterMapLayer> pMapLayer;
	NGfx::SPixel8888 color;
	int nWidth, nHeight;
	int nLineWidth;
	CArray2D1Bit *pMask;
	int nMaskWidth, nMaskHeight;

	void DrawOctoCircle( int nX0, int nY0, float fR );
public:
	CDrawMapPixelFunctional( CWindowChapterMapLayer *_pMapLayer, NGfx::SPixel8888 _color, CArray2D1Bit *_pMask, int _nLineWidth = 0 )	// Negative DashLen gives solid wide line
		: pMapLayer( _pMapLayer ), color( _color ), nLineWidth( _nLineWidth ), pMask( _pMask )
	{
		nWidth = pMapLayer->GetWidth();
		nHeight = pMapLayer->GetHeight();
		if ( pMask )
		{
			nMaskWidth = pMask->GetSizeX();
			nMaskHeight = pMask->GetSizeY();
		}
		else
		{
			nMaskWidth = 0;
			nMaskHeight = 0;
		}
	}

	void operator()( int x, int y )
	{
		if ( x >= 0 && y >= 0 && x < nWidth - 1 && y < nHeight - 1 )
		{
			if ( nMaskWidth > 0 || nMaskHeight > 0 )		// Ignore empty mask
			{
				if ( x >= nMaskWidth || y >= nMaskHeight || pMask->GetData( x, y ) )
					return;		
			}

			if ( nLineWidth == 0 )
				pMapLayer->PutPixel( x, y, color );
			else
				DrawOctoCircle( x, y, nLineWidth * 0.5f );
		}
	}
};

class CWindowPotentialLines: public CWindow, public IPotentialLines
{
	OBJECT_BASIC_METHODS( CWindowPotentialLines );

	enum EIntersectionType { 
		EIT_SEA, EIT_NONE, 
		EIT_CORNER_NE, EIT_CORNER_SE, EIT_CORNER_SW, EIT_CORNER_NW,
		EIT_LINE_V, EIT_LINE_H,
		EIT_CROSS
	};

	struct SPointDesc
	{
		ZDATA
		float fValue;
		EIntersectionType eType;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&fValue); f.Add(3,&eType); return 0; }
	};
	typedef CArray2D<SPointDesc> CPointsArray;

	struct SNodeDesc
	{
		ZDATA
		int nX;
		int nY;
		float fValue;
		int nEndOffsetX;
		int nEndOffsetY;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nX); f.Add(3,&nY); f.Add(4,&fValue); f.Add(5,&nEndOffsetX); f.Add(6,&nEndOffsetY); return 0; }
	};
	typedef list<SNodeDesc> CNodeList;

	struct SArrowDesc 
	{
		ZDATA
		vector<CVec2> pts;
		float fWidth;
		DWORD dwColour;
		CDBPtr<NDb::STexture> pTexture;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pts); f.Add(3,&fWidth); f.Add(4,&dwColour); f.Add(5,&pTexture); return 0; }
	};
	typedef list<SArrowDesc> CArrowList;

	// This data is generated
	CPointsArray pts;
	CObj<CWindowChapterMapLayer> pLinesLayer;
	CObj<CWindowChapterMapLayer> pTerritoryLayer;
	CArray2D<float> noise;
	CArray2D1Bit seaMask;
	CArrowList arrows;

	ZDATA_(CWindow)
	CPtr<NDb::SWindowPotentialLines> pInstance;
	CDBPtr<NDb::SWindowPotentialLinesShared> pShared;

	CNodeList nodes;
	bool bValid;
	bool bLayerInit;
	bool bDrawDashes;

	DWORD colourBorder1;		// our colour
	DWORD colourBorder2;
	CVec2 vTextureSize;

	CVec2 vMainStrike;
	string szMaskFile;
	string szDiffColourMapFile;

	ZONSERIALIZE
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CWindow*)this); f.Add(2,&pInstance); f.Add(3,&pShared); f.Add(4,&nodes); f.Add(5,&bValid); f.Add(6,&bLayerInit); f.Add(7,&bDrawDashes); f.Add(8,&colourBorder1); f.Add(9,&colourBorder2); f.Add(10,&vTextureSize); f.Add(11,&vMainStrike); f.Add(12,&szMaskFile); f.Add(13,&szDiffColourMapFile); OnSerialize( f ); return 0; }
protected:
	NDb::SWindow* GetInstance() { return pInstance; }

private:
	enum ENeighbourSide { ENS_UP, ENS_RIGHT, ENS_DOWN, ENS_LEFT };

	void Recalculate();
	const float GetValue( int nX, int nY ) const;
	void DrawArrows( struct IUIVisitor *pVisitor );
	void DrawNoise( const CVec2 &vMainStrike );
	void SetupNoise();
	float GetSquaredDistanceToSegment( const SNodeDesc &node, int nX, int nY ) const;

	void OnSerialize( IBinSaver &f );
public:
	CWindowPotentialLines();

	//{ CWindow
	void Visit( struct IUIVisitor *pVisitor );
	void InitByDesc( const struct NDb::SUIDesc *pDesc ); 
	//}

	void SetParams( const string &szMask, const string &szDiffColourMap, const CVec2 &_vMainStrike, const DWORD _dwBorderColour1, const DWORD _dwBorderColour2 );

	// Nodes management
	void ClearNodes();
	void SetNode( int nX, int nY, int nEndOffsetX, int nEndOffsetY, float fValue );		// If such node exists, it is altered

	// Arrows management
	void ClearArrows() { arrows.clear(); }
	void AddArrow( const vector<CVec2> &arrowTraj, float fArrowWidth, const NDb::STexture *pArrowTexture, DWORD dwArrowColour );
};

