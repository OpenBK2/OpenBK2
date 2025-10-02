#pragma once
#include "RectLayout.h"
#include "GLocale.h"

class CFontFormatInfo;
namespace NGScene
{

struct SText
{
	struct SCharRect
	{
		int nIndex;
		CTRect<int> sRect;

		SCharRect() {}
		SCharRect( int _nIndex, CTRect<int> _sRect ):
			nIndex( _nIndex ), sRect( _sRect ) {}
	};
	
	struct SFontLayout
	{
		ZDATA
		CRectLayout sLayout;
		
		SFont sFont;
		CPtr<CFontInfo> pFontInfo;
		ZEND int operator&( CStructureSaver &f ) { f.Add(2,&sLayout); f.Add(3,&sFont); f.Add(4,&pFontInfo); return 0; }

		SFontLayout() {}
		SFontLayout( CFontInfo *pInfo, const CRectLayout &_sLayout ): sLayout( _sLayout ), pFontInfo( pInfo ) {}
	};

	ZDATA
	CTPoint<int> sSize;
	vector<SCharRect> charsSet;
	vector<SFontLayout> rectLayouts;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&sSize); f.Add(3,&charsSet); f.Add(4,&rectLayouts); return 0; }
};

CFuncBase<SText>* CreateTextFormater( CTextLocaleInfo *pInfo, CFuncBase<CVec2> *pScreenRect, CFuncBase<wstring> *pText, CFuncBase< CTPoint<int> > *pSize, bool bProcessTAGs = true );

}; // namespace 


