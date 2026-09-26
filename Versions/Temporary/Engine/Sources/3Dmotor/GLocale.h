#pragma once
#include "3Dmotor_export.h"


#include "FontFormat.h"
#include "System/Dg.h"

namespace NDb
{
	struct SFont;
}
namespace NGfx
{
	class CTexture;
};
namespace NGScene
{

//! Типы шрифтов
struct SFont
{
	ZDATA
	int nSize;
	std::string szName;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&nSize); f.Add(3,&szName); return 0; }

	SFont() {}
	SFont( int _nSize, const std::string &_szName ): nSize( _nSize ), szName( _szName ) {}
};

//! Описание шрифта
class CFontInfo: public CObjectBase
{
	OBJECT_BASIC_METHODS(CFontInfo);
private:
	ZDATA
	SFont sFont;
	CObj< CPtrFuncBase<NGfx::CTexture> > pTexture;
	CDGPtr< CPtrFuncBase<CFontFormatInfo> > pInfo;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&sFont); f.Add(3,&pTexture); f.Add(4,&pInfo); return 0; }
		
public:
	CFontInfo() {}
	CFontInfo( const SFont &_sFont, CPtrFuncBase<NGfx::CTexture> *_pTexture, CPtrFuncBase<CFontFormatInfo> *_pInfo ):
		sFont( _sFont ), pTexture( _pTexture ), pInfo( _pInfo ) {}

	const SFont& GetType() const { return sFont; }
	CPtrFuncBase<NGfx::CTexture>* GetTexture() const { return pTexture; }
	CPtrFuncBase<CFontFormatInfo>* GetFormatInfo() const { return pInfo; }
};

//! Локаль
class _3DMOTOR_EXPORT CTextLocaleInfo: public CObjectBase
{
	OBJECT_BASIC_METHODS(CTextLocaleInfo);
private:
	ZDATA
	CVec2 vScreenRect;
	std::vector< CObj<CFontInfo> > fonts;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&vScreenRect); f.Add(3,&fonts); return 0; }

protected:
	CFontInfo* SearchFont( const SFont &sFont );

public:
	CTextLocaleInfo();

	void ClearAllFonts() { fonts.clear(); }
	void AddFont( const NDb::SFont *pFont );
	void AddAllAvailableFonts();

	void Setup( const CVec2 &vScreenRect );

	virtual CFontInfo* GetFont( const SFont &sFont );
};

// Converts a font size in points to the pixel size to search for and draw at.
//
// Points are defined against the 1024x768 virtual screen the UI is laid out on,
// which the screen stretches with independent X and Y factors. This used to take
// the X factor and correct only scale.y for aspect, so on anything wider than
// 4:3 every glyph came out stretched horizontally: at 2560x1600, 2.5 across and
// 2.08 down. Taking the Y factor instead, and scaling both axes by the same
// amount, keeps glyphs in proportion; a wider screen gives text more room across
// rather than wider letters. The vertical size is what it was before.
//
// Truncates to whole pixels, as before, because the font search is by integer
// height and an atlas can only be drawn texel for pixel at an integer size.
inline int FontPointsToPixels( const int nPoints, const float fScreenHeight )
{
	return (float)nPoints * fScreenHeight / 768.0f;
}

}; // namespace 


