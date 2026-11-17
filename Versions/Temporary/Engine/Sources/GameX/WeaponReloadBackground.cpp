#include "stdafx.h"

#include "3Dmotor/RectLayout.h"
#include "Main/GameTimer.h"
#include "UI/UIVisitor.h"
#include "WeaponReloadBackground.h"

void CWeaponReloadBackground::SetReloads( const std::vector<SWeaponReloadInfo> &values, uint32_t nTint )
{
	reloads = values;
	// Missing legacy binary fields and transparent authored values retain a visible default.
	nColor = ( nTint >> 24 ) != 0 ? nTint : 0x80404040;
}

void CWeaponReloadBackground::SetPos( const CVec2 &vPos, const CVec2 &vSize )
{
	CBackground::SetPos( vPos, vSize );
	if ( pOriginal )
	{
		pOriginal->SetPos( vPos, vSize );
	}
}

void CWeaponReloadBackground::SetTexture( const NDb::STexture *pTexture )
{
	if ( pOriginal )
	{
		pOriginal->SetTexture( pTexture );
	}
}

void CWeaponReloadBackground::SetOutline( const CDBID &outlineType )
{
	if ( pOriginal )
	{
		pOriginal->SetOutline( outlineType );
	}
}

void CWeaponReloadBackground::SetFontScaleWindow( IWindow *pWindow )
{
	if ( pOriginal )
	{
		pOriginal->SetFontScaleWindow( pWindow );
	}
}

void CWeaponReloadBackground::Visit( IUIVisitor *pVisitor )
{
	if ( pOriginal )
	{
		pOriginal->SetFadeValue( GetFadeValue() );
		pOriginal->Visit( pVisitor );
	}
	const IGameTimer *pTimer = GameTimer();
	if ( !pTimer || reloads.empty() || pos.IsEmpty() )
	{
		return;
	}
	CRectLayout rects;
	const float fWidth = pos.Width() / reloads.size();
	for ( size_t i = 0; i < reloads.size(); ++i )
	{
		const float fRemaining = reloads[i].GetRemainingFraction( pTimer->GetGameTime() );
		if ( fRemaining > 0.0f )
		{
			// Keep the remaining shade at the top, revealing the icon from bottom to top.
			rects.AddRect( pos.x1 + i * fWidth,
			               pos.y1,
			               fWidth,
			               pos.Height() * fRemaining,
			               CTRect<float>( 0, 0, 0, 0 ),
			               FadeColor( nColor, GetFadeValue() ) );
		}
	}
	if ( !rects.rects.empty() )
	{
		VirtualToScreen( &rects );
		pVisitor->VisitUIRect( nullptr, 3, rects );
	}
}

int CWeaponReloadBackground::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CBackground *>( this ) );
	saver.Add( 2, &pOriginal );
	saver.Add( 3, &reloads );
	saver.Add( 4, &nColor );
	return 0;
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x20261118, CWeaponReloadBackground )
