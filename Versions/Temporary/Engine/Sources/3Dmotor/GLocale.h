#pragma once
#include "3Dmotor_export.h"


#include "FontFormat.h"
#include "System/Dg.h"

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <tuple>

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
	// Horizontal cell size in screen pixels; zero means the same as nSize.
	// Runtime fonts rasterise both axes, preserving the original UI proportions.
	int nWidth = 0;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&nSize); f.Add(3,&szName); f.Add(4,&nWidth); return 0; }

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
	// Called with the text about to be laid out, before its characters are
	// looked up. A baked font has every glyph it will ever have; a runtime font
	// (GRuntimeFont.h) rasterises the ones it lacks here.
	virtual void PrepareGlyphs( const std::wstring &wsText ) {}
	// A runtime font is already rasterised at the requested screen dimensions.
	virtual bool IsRasterized() const { return false; }
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

	// Runtime fonts, which are not saved: a save game keeps the atlases its
	// text uses, and anything else is made again the first time it is asked
	// for. Records by font name, for the names whose record has a FontFile; and
	// the fonts made so far by name and pixel size, a null entry marking one
	// whose font file could not be used, so the baked font answers from then on.
	std::unordered_map<std::string, const NDb::SFont*> runtimeRecords;
	std::map<std::tuple<std::string, int, int>, CObj<CFontInfo>> runtimeFonts;
	float fCachedRuntimeFontScale = 0;
	CFontInfo* GetRuntimeFont( const SFont &sFont );

protected:
	CFontInfo* SearchFont( const SFont &sFont );

public:
	CTextLocaleInfo();

	void ClearAllFonts();
	void AddFont( const NDb::SFont *pFont );
	void AddAllAvailableFonts();

	void Setup( const CVec2 &vScreenRect );

	virtual CFontInfo* GetFont( const SFont &sFont );
};

// Effective NGlobal ui_font_scale, shared by rasterisation and layout caches.
_3DMOTOR_EXPORT float GetRuntimeFontScale();

// Font sizes follow the original 1024x768 UI coordinate system. Keep both
// dimensions: runtime fonts rasterise this aspect ratio into their atlas, so
// restoring the old widescreen width does not reintroduce bitmap filtering.
inline int FontPointsToPixels( const int nPoints, const float fScreenHeight )
{
	return (float)nPoints * fScreenHeight / 768.0f;
}

inline int FontPointsToPixelWidth( const int nPoints, const float fScreenWidth )
{
	return (float)nPoints * fScreenWidth / 1024.0f;
}

}; // namespace 


