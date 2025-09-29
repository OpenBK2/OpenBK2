#ifndef __GLOCALE_H__
#define __GLOCALE_H__
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "FontFormat.h"
#include "..\System\DG.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////////////////////////
//! Типы шрифтов
struct SFont
{
	ZDATA
	int nSize;
	string szName;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&nSize); f.Add(3,&szName); return 0; }

	SFont() {}
	SFont( int _nSize, const string &_szName ): nSize( _nSize ), szName( _szName ) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////////////////////////
//! Локаль
class CTextLocaleInfo: public CObjectBase
{
	OBJECT_BASIC_METHODS(CTextLocaleInfo);
private:
	ZDATA
	CVec2 vScreenRect;
	vector< CObj<CFontInfo> > fonts;
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
////////////////////////////////////////////////////////////////////////////////////////////////////
}; // namespace 
////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
