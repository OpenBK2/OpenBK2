#pragma once

#include "3Dmotor/GfxBuffers.h"
#include "System/Dg.h"

#include <cstdint>

struct IML;
namespace NGScene
{
	class I2DGameView;
}
namespace NGfx
{
class CTexture;
enum ETextureUsage;
}
namespace NDb
{
	struct STexture;
}

struct SFormattingInfo
{
	enum EType
	{
		ELEM_TEXT,
		ELEM_PICTURE,
		ELEM_OUTLINE,
		ELEM_GRAY_FILTER
	};
	enum ECenterFlags
	{
		CENTER_X = 1,
		CENTER_Y = 2,
	};
	struct SCmd
	{
		ZDATA
		std::wstring szText;
		CVec2 vPos;
		CVec2 vSize;
		uint32_t dwColor;
		CDBPtr<NDb::STexture> pTexture;
		CDBID nOutlineType;
		int nFlags; // CENTER_
		int nChainX;
		EType type;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&szText); f.Add(3,&vPos); f.Add(4,&vSize); f.Add(5,&dwColor); f.Add(6,&pTexture); f.Add(7,&nOutlineType); f.Add(8,&nFlags); f.Add(9,&nChainX); f.Add(10,&type); return 0; }

		SCmd() : type(ELEM_TEXT), vPos(0,0), vSize(0,0), dwColor(0), nFlags(0), nChainX(-1) {}
		SCmd( const std::wstring &_szText, const CVec2 &_vPos, int _nFlags )
			: type(ELEM_TEXT), vPos(_vPos), vSize(0,0), dwColor(0), szText(_szText), nFlags(_nFlags), nChainX(-1) {}
		SCmd( const NDb::STexture *_pTexture, const CVec2 &_vPos, const CVec2 &_vSize, uint32_t _dwColor )
			: type(ELEM_PICTURE), pTexture(_pTexture), vPos(_vPos), vSize(_vSize), dwColor(_dwColor), nFlags(0), nChainX(-1) {}
		SCmd( const CDBID &_nOutlineType ) : type(ELEM_OUTLINE), nOutlineType(_nOutlineType), vPos(0,0), vSize(0,0), dwColor(0), nFlags(0), nChainX(-1) {}
		SCmd( uint32_t _dwColor ) : type(ELEM_GRAY_FILTER), vPos(0,0), vSize(0,0), dwColor(_dwColor), nFlags(0), nChainX(-1) {}
		bool operator==( const SCmd &a ) const 
		{ 
			return szText == a.szText && vPos == a.vPos && vSize == a.vSize && dwColor == a.dwColor && 
				pTexture == a.pTexture && nOutlineType == a.nOutlineType && nFlags == a.nFlags && nChainX == a.nChainX &&
				type == a.type;
		}
	};

	ZDATA
	std::vector<SCmd> cmds;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&cmds); return 0; }
	bool operator==( const SFormattingInfo &a ) const { return cmds == a.cmds;}
	void AddText( const std::wstring &_szText, const CVec2 &_vPos = CVec2(0,0), int _nFlags = 0 ) { cmds.push_back( SCmd( _szText, _vPos, _nFlags ) ); }
	void AddPicture( const NDb::STexture *_pTexture, const CVec2 &_vPos = CVec2(0,0), const CVec2 &_vSize = CVec2(0,0), uint32_t _dwColor = 0xffffffff )
	{
		cmds.push_back( SCmd( _pTexture, _vPos, _vSize, _dwColor ) );
	}
	void AddOutline( const CDBID &_nOutlineType ) { cmds.push_back( SCmd( _nOutlineType ) ); }
	void AddGrayFilter( uint32_t _dwColor ) { cmds.push_back( SCmd( _dwColor ) ); }
	void ChainLastCmd( int nShift = 1 ) { int nCmds = cmds.size(); cmds[ nCmds - 1 ].nChainX = nCmds - 1 - nShift; }
};

class CIconOutliner : public CPtrFuncBase<NGfx::CTexture>
{
	OBJECT_NOCOPY_METHODS(CIconOutliner);
	ZDATA
	SFormattingInfo fmt, fmtProcessed;
	std::vector<CObj<IML> > gfxTexts;
	bool bNeedUpdate;
	NGfx::ETextureUsage eUsage;
	CDBID nOutlineType;
	CTPoint<int> fixedSize, size;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&fmt); f.Add(3,&fmtProcessed); f.Add(4,&gfxTexts); f.Add(5,&bNeedUpdate); f.Add(6,&eUsage); f.Add(7,&nOutlineType); f.Add(8,&fixedSize); f.Add(9,&size); return 0; }

	bool NeedUpdate();
	void Recalc();

public:
	CIconOutliner( CDBID nOutlineType = CDBID(), NGfx::ETextureUsage _eUsage=NGfx::TEXTURE_2D, const CTPoint<int> &size = CTPoint<int>(0,0) );
	
	const CTPoint<int> &GetSize() { return size; };
	void SetFormat( const SFormattingInfo &_fmt );
	void SetText( const std::wstring &wszText )
	{
		SFormattingInfo inf;
		inf.AddText( wszText );
		SetFormat( inf );
	}
	void SetTexture( const NDb::STexture *pTexture, const CVec2 &vPos=CVec2( 0, 0 ) )
	{
		SFormattingInfo inf;
		inf.AddPicture( pTexture, vPos );
		SetFormat( inf );
	}
	static CVec2 GetOutlineAdd( const CDBID &nOutlineType );
	static CVec2 GetSizeAdd() { return CVec2( 16, 16 ); }
	static float GetTextStretchX() { return 1; }
};

