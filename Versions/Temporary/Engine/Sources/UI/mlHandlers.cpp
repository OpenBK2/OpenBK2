#include "stdafx.h"
#include "Misc/StrProc.h"
#include "3Dmotor/RectLayout.h"
#include "3Dmotor/G2DView.h"
#include "3Dmotor/Locale.h"
#include "3Dmotor/GLocale.h"
//
#include "mlMain.h"
#include "mlReflow.h"
#include "mlObjects.h"
#include "mlVisObjects.h"
#include "mlHandlers.h"

namespace NML
{

static NGfx::SPixel8888 StringToColor( const std::wstring &color, float fFade = 1.0f )
{
	NGfx::SPixel8888 retColor;

	if ( color.compare( L"white" ) == 0 )
		retColor = NGfx::SPixel8888( 0xFF, 0xFF, 0xFF, 0xFF );
	else if ( color.compare( L"red" ) == 0 )
		retColor = NGfx::SPixel8888( 0xFF, 0, 0, 0xFF );
	else if ( color.compare( L"green" ) == 0 )
		retColor = NGfx::SPixel8888( 0, 0xFF, 0, 0xFF );
	else if ( color.compare( L"blue" ) == 0 )
		retColor = NGfx::SPixel8888( 0, 0, 0xFF, 0xFF );
	else if ( color.compare( L"yellow" ) == 0 )
		retColor = NGfx::SPixel8888( 0xFF, 0xFF, 0, 0xFF );
	else if ( color.compare( L"cyan" ) == 0 )
		retColor = NGfx::SPixel8888( 0, 0xFF, 0xFF, 0xFF );
	else if ( color.compare( L"orange" ) == 0 )
		retColor = NGfx::SPixel8888( 0xFF, 0x80, 0x40, 0xFF );
	else if ( color.compare( L"pink" ) == 0 )
		retColor = NGfx::SPixel8888( 0xFF, 0x80, 0xFF, 0xFF );
	else if ( color.compare( L"brown" ) == 0 )
		retColor = NGfx::SPixel8888( 110, 110, 56, 0xFF );
	else if ( color.compare( L"grey" ) == 0 )
		retColor = NGfx::SPixel8888( 180, 180, 180, 0xFF );
	else if ( color.compare( L"black" ) == 0 )
		retColor = NGfx::SPixel8888( 0, 0, 0, 0xFF );
	else if ( color.compare( L"darkyellow" ) == 0 )
		retColor = NGfx::SPixel8888( 242, 192, 17, 0xFF );
	else if ( color.compare( L"beige" ) == 0 )
		retColor = NGfx::SPixel8888( 209, 183, 167, 0xFF );
	else if ( color.compare( L"lightblue" ) == 0 )
		retColor = NGfx::SPixel8888( 0, 128, 250, 0xFF );
	else
		swscanf( color.c_str(), L"%x", &retColor.dwColor );
	if ( fFade != 1.0f )
	{
		retColor.a *= fFade;
		retColor.r *= fFade;
		retColor.g *= fFade;
		retColor.b *= fFade;
	}
	return retColor;
}

// CBRHandler

void CBRHandler::Exec( CMLStream *pStream, IReflowLayout *pLayout, const std::vector<std::wstring> &paramsSet )
{
	pLayout->AddObject( CreateLineBreakObject() );
}

// CTabHandler

void CTabHandler::Exec( CMLStream *pStream, IReflowLayout *pLayout, const std::vector<std::wstring> &paramsSet )
{
	pLayout->AddObject( CreateTabObject() );
}

// CColorHandler

void CColorHandler::Exec( CMLStream *pStream, IReflowLayout *pLayout, const std::vector<std::wstring> &paramsSet )
{
	if ( paramsSet.size() != 3 )
		return;
	pLayout->AddObject( CreateColorObject( StringToColor( paramsSet[2], sFadeValue->fFadeValue ) ) );
}

// CFontHandler

void CFontHandler::Exec( CMLStream *pStream, IReflowLayout *pLayout, const std::vector<std::wstring> &paramsSet )
{
	int nFlags = 0;
	int nOutlineSize = 0;
	bool bForceFontSize = false;
	NGScene::SFont font;
	NGfx::SPixel8888 outlineColor;

	int nTemp = 1;
	while( nTemp < paramsSet.size() )
	{
		if ( nTemp + 2 >= paramsSet.size() )
			break;
		if ( paramsSet[nTemp + 1].compare( L"=" ) != 0 )
			break;

		const std::wstring &wsID = paramsSet[nTemp];
		const std::wstring &wsParam = paramsSet[nTemp + 2];
		if ( wsID.compare( L"size" ) == 0  )
		{
			nFlags |= N_FONTOBJECT_SIZE;

			wchar_t wsString[128];
			int nParams = swscanf( wsParam.c_str(), L"%d%2s", &font.nSize, wsString );

			if ( nParams > 1 )
			{
				std::wstring sizeMod( wsString );
				if ( sizeMod.compare( L"px" ) == 0 )
					font.nSize |= FONT_SIZE_PIXELS;
				else if ( sizeMod.compare( L"pt" ) == 0 )
					font.nSize |= FONT_SIZE_POINTS;
			}
			else
				font.nSize |= FONT_SIZE_POINTS;

		}
		else if ( wsID.compare( L"face" ) == 0  )
		{
			nFlags |= N_FONTOBJECT_NAME;
			font.szName = WideToUTF8( wsParam );
		}
		else if ( wsID.compare( L"outlinesize" ) == 0  )
		{
			nFlags |= N_FONTOBJECT_OUTLINESIZE;
			nOutlineSize = NStr::ToIntDec( wsParam );
		}
		else if ( wsID.compare( L"outlinecolor" ) == 0  )
		{
			nFlags |= N_FONTOBJECT_OUTLINECOLOR;
			outlineColor = StringToColor( wsParam );
		}
		else if ( wsID.compare( L"forcefontsize" ) == 0  )
		{
			nFlags |= N_FONTOBJECT_FORCEFONTSIZE;
			bForceFontSize = true;
		}

		nTemp += 3;
	}

	pLayout->AddObject( CreateFontObject( nFlags, font, outlineColor, nOutlineSize, bForceFontSize ) );
}

// CMinFontSizeHandler

void CMinFontSizeHandler::Exec( CMLStream *pStream, IReflowLayout *pLayout, const std::vector<std::wstring> &paramsSet )
{
	if ( paramsSet.size() != 4 )
		return;
	if ( paramsSet[1].compare( L"size" ) == 0  )
		return;
	if ( paramsSet[2].compare( L"=" ) == 0  )
		return;

	int nMinFontSize = NStr::ToIntDec( paramsSet[3] );
	pLayout->AddObject( CreateMinFontSizeObject( nMinFontSize ) );
}

} // NAMESPACE

using namespace NML;
REGISTER_SAVELOAD_CLASS( UI, 0xB5529210, CBRHandler )
REGISTER_SAVELOAD_CLASS( UI, 0xB5529211, CTabHandler )
REGISTER_SAVELOAD_CLASS( UI, 0xB5529212, CColorHandler )
REGISTER_SAVELOAD_CLASS( UI, 0xB5529213, CFontHandler )
REGISTER_SAVELOAD_CLASS( UI, 0xB5529214, CMinFontSizeHandler )
REGISTER_SAVELOAD_CLASS( UI, 0xB5529215, CLEFTHandler )
REGISTER_SAVELOAD_CLASS( UI, 0xB5529216, CRIGHTHandler )
REGISTER_SAVELOAD_CLASS( UI, 0xB5529217, CCENTERHandler )
REGISTER_SAVELOAD_CLASS( UI, 0xB5529218, CNOWRAPHandler )
REGISTER_SAVELOAD_CLASS( UI, 0xB5529219, CJUSTIFYHandler )
REGISTER_SAVELOAD_CLASS( UI, 0xB552921A, CWRAPLEFTHandler )
REGISTER_SAVELOAD_CLASS( UI, 0xB552921B, CWRAPRIGHTHandler )
REGISTER_SAVELOAD_CLASS( UI, 0xB552921C, CTOPHandler )
REGISTER_SAVELOAD_CLASS( UI, 0xB552921D, CMIDDLEHandler )
REGISTER_SAVELOAD_CLASS( UI, 0xB552921E, CBOTTOMHandler )

