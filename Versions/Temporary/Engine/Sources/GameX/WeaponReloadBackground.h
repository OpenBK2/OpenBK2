#pragma once

#include "Stats_B2_M1/WeaponReloadInfo.h"
#include "UI/Background.h"

#include "GameX_export.h"

#include <cstdint>
#include <vector>

// Decorate the existing weapon icon, retaining its texture, placement and input handling.
class GAMEX_EXPORT CWeaponReloadBackground : public CBackground
{
	OBJECT_BASIC_METHODS( CWeaponReloadBackground );
	CObj<IWindowPart> pOriginal;
	std::vector<SWeaponReloadInfo> reloads;
	uint32_t nColor = 0x80404040;

public:
	CWeaponReloadBackground() {}

	explicit CWeaponReloadBackground( IWindowPart *pBackground )
		: pOriginal( pBackground )
	{
	}

	void SetReloads( const std::vector<SWeaponReloadInfo> &values, uint32_t nTint );
	void SetPos( const CVec2 &vPos, const CVec2 &vSize ) override;
	void SetTexture( const NDb::STexture *pTexture ) override;
	void SetOutline( const CDBID &outlineType ) override;
	void SetFontScaleWindow( IWindow *pWindow ) override;
	void Visit( IUIVisitor *pVisitor ) override;
	int operator&( IBinSaver &saver ) override;
};
