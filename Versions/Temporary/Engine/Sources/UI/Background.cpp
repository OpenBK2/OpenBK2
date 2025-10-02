// Background.cpp: implementation of the CBackgroundSimpleScallingTexture class.
//


#include "stdafx.h"
#include "Background.h"

DWORD FadeColor( DWORD dwColor, float fFade )
{
	return DWORD( (dwColor & 0xFF ) * fFade ) |
		DWORD( (dwColor >> 8 & 0xFF ) * fFade ) << 8 |
		DWORD( (dwColor >> 16 & 0xFF ) * fFade ) << 16 |
		DWORD( (dwColor >> 24 & 0xFF ) * fFade ) << 24;
//	return (dwColor & 0x00FFFFFF) |
//		DWORD( (dwColor >> 24 & 0xFF ) * fFade ) << 24; // вариант с изменением только альфы смотрится хуже
}

BASIC_REGISTER_CLASS(IWindowPart)
using namespace NGScene;

//CBackground

CBackground::CBackground() :
	fFadeValue( 1.0f )
{
}

void CBackground::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	pos.left = 0;
	pos.top = 0;
	pos.bottom = 0;
	pos.right = 0;
}

void CBackground::SetPos( const CVec2 &vPos, const CVec2 &vSize )
{
	pos.left = vPos.x;
	pos.top = vPos.y;
	pos.bottom = vPos.y + vSize.y;
	pos.right = vPos.x + vSize.x;
}

int CBackground::operator&( interface IBinSaver &saver )
{
	saver.Add( 1, &pos );
	saver.Add( 2, &fFadeValue );
	return 0;
}
