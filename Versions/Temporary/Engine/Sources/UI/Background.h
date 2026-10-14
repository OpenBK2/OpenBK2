// Background.h: interface for the CBackground class.
//

#pragma once

#include "UI.h"

#include "UI_export.h"

#include <cstdint>

namespace NGScene
{
class CFileTexture;
};

// fFade[0..1]
UI_EXPORT uint32_t FadeColor( uint32_t dwColor, float fFade );

class UI_EXPORT CBackground : public IWindowPart
{
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	float fFadeValue = {};

protected:
	CTRect<float> pos;

protected:
	float GetFadeValue() const
	{
		return fFadeValue;
	}

public:
	CBackground();

	void SetPos( const CVec2 &vPos, const CVec2 &vSize ) override;
	int operator&( struct IBinSaver &ss ) override;

	void Init() override {}

	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;

	// return true if pixel under vPos is visible (alpha is not full)
	virtual bool IsVisiblePixel( const CVec2 &vPos ) const
	{
		return true;
	}

	void SetFadeValue( float fValue ) override
	{
		fFadeValue = fValue;
	}
};
