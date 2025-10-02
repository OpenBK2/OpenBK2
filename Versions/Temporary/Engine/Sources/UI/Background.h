// Background.h: interface for the CBackground class.
//


#if !defined(AFX_BACKGROUND_H__E331DA1D_E450_4271_9D2D_E39295F8A0AF__INCLUDED_)
#define AFX_BACKGROUND_H__E331DA1D_E450_4271_9D2D_E39295F8A0AF__INCLUDED_

#include "UI_export.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "UI.h"

namespace NGScene
{
	class CFileTexture;
};

// fFade[0..1]
UI_EXPORT DWORD FadeColor( DWORD dwColor, float fFade );

class UI_EXPORT CBackground : public IWindowPart
{
	float fFadeValue;
protected:
	CTRect<float> pos;
protected:
	float GetFadeValue() const { return fFadeValue; }
public:
	CBackground();

	virtual void SetPos( const CVec2 &vPos, const CVec2 &vSize );
	virtual int operator&( interface IBinSaver &ss );
	virtual void Init() {  }
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );

	// return true if pixel under vPos is visible (alpha is not full)
	virtual bool IsVisiblePixel( const CVec2 &vPos ) const { return true; }

	void SetFadeValue( float fValue ) { fFadeValue = fValue; }
};

#endif // !defined(AFX_BACKGROUND_H__E331DA1D_E450_4271_9D2D_E39295F8A0AF__INCLUDED_)

