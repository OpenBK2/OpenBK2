#pragma once
#include "Window.h"

class CWindowTooltip : public CWindow, public ITooltip
{
	OBJECT_BASIC_METHODS(CWindowTooltip)

	ZDATA_( CWindow )
		CDBPtr<NDb::SWindowTooltipShared> pShared;
		ZSKIP //bool bEffectRunning;
		ZSKIP //bool bAppeared;
		CPtr<NDb::SWindowTooltip> pInstance;
		bool bInitializedByText;
		int nMaxXPos;
		int nDesiredYPos;
		std::wstring wszText;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CWindow *)this); f.Add(2,&pShared); f.Add(5,&pInstance); f.Add(6,&bInitializedByText); f.Add(7,&nMaxXPos); f.Add(8,&nDesiredYPos); f.Add(9,&wszText); return 0; }
protected:
	virtual NDb::SWindow* GetInstance() { return pInstance; }

public:
	CWindowTooltip() : bInitializedByText( false ) { }
	// {CRAP: for compatibility with H5 ugly tooltips
	CWindowTooltip( const std::wstring &_wszText ) : wszText( _wszText ), bInitializedByText( true ) { }
	// CRAP}

	// return position on screen 
	virtual void InitTooltip( const CVec2 &vPos, const CTRect<float> &wndRect, const std::wstring &szText,
		IScreen *pScreen, const int nTooltipWidth, const float fHorisontalToVerticalRatio,
		int nIDForMLHandler );
	void AdjustPosByMousePos( const CVec2 &vMousePos );

	void InitByDesc( const struct NDb::SUIDesc* pDesc );

	const std::wstring &GetText() const { return wszText; }
};


