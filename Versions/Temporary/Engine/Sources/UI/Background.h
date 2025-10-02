// Background.h: interface for the CBackground class.
//



#include "UI_export.h"

#pragma once
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
	virtual int operator&( struct IBinSaver &ss );
	virtual void Init() {  }
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );

	// return true if pixel under vPos is visible (alpha is not full)
	virtual bool IsVisiblePixel( const CVec2 &vPos ) const { return true; }

	void SetFadeValue( float fValue ) { fFadeValue = fValue; }
};


