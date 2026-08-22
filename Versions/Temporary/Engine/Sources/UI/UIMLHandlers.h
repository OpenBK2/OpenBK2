#pragma once
#include "UIML.h"

inline NGfx::SPixel8888 StringToColor( const std::wstring &wsColor, float fFade = 1.0f )
{
	NGfx::SPixel8888 sColor;

	if ( wsColor.compare( L"white" ) == 0 )
		sColor = NGfx::SPixel8888( 0xFF, 0xFF, 0xFF, 0xFF );
	else if ( wsColor.compare( L"red" ) == 0 )
		sColor = NGfx::SPixel8888( 0xFF, 0, 0, 0xFF );
	else if ( wsColor.compare( L"green" ) == 0 )
		sColor = NGfx::SPixel8888( 0, 0xFF, 0, 0xFF );
	else if ( wsColor.compare( L"blue" ) == 0 )
		sColor = NGfx::SPixel8888( 0, 0, 0xFF, 0xFF );
	else if ( wsColor.compare( L"yellow" ) == 0 )
		sColor = NGfx::SPixel8888( 0xFF, 0xFF, 0, 0xFF );
	else if ( wsColor.compare( L"cyan" ) == 0 )
		sColor = NGfx::SPixel8888( 0, 0xFF, 0xFF, 0xFF );
	else if ( wsColor.compare( L"orange" ) == 0 )
		sColor = NGfx::SPixel8888( 0xFF, 0x80, 0x40, 0xFF );
	else if ( wsColor.compare( L"pink" ) == 0 )
		sColor = NGfx::SPixel8888( 0xFF, 0x80, 0xFF, 0xFF );
	else if ( wsColor.compare( L"brown" ) == 0 )
		sColor = NGfx::SPixel8888( 110, 110, 56, 0xFF );
	else if ( wsColor.compare( L"grey" ) == 0 )
		sColor = NGfx::SPixel8888( 180, 180, 180, 0xFF );
	else if ( wsColor.compare( L"black" ) == 0 )
		sColor = NGfx::SPixel8888( 0, 0, 0, 0xFF );
	else if ( wsColor.compare( L"darkyellow" ) == 0 )
		sColor = NGfx::SPixel8888( 242, 192, 17, 0xFF );
	else if ( wsColor.compare( L"beige" ) == 0 )
		sColor = NGfx::SPixel8888( 209, 183, 167, 0xFF );
	else if ( wsColor.compare( L"lightblue" ) == 0 )
		sColor = NGfx::SPixel8888( 0, 101, 213, 0xFF );
	else
		swscanf( wsColor.c_str(), L"%x", &sColor.dwColor );

	if ( fFade != 1.0f )
	{
		sColor.a *= fFade;
		sColor.r *= fFade;
		sColor.g *= fFade;
		sColor.b *= fFade;
	}

	return sColor;
}

#define DECALRE_SIMPLE_HANDLER( Name, Var, Value) \
class Name: public IMLHandler											\
{																									\
	OBJECT_BASIC_METHODS( Name );										\
private:																					\
	ZDATA																						\
	ZEND int operator&( IBinSaver &f ) { return 0; }\
public:																						\
void Exec( IML *pIML, IMLLayout *pLayout, const std::vector<std::wstring> &paramsSet )	\
	{																								\
		SState sState = pLayout->GetState();					\
		sState.Var = Value;														\
		pLayout->SetState( sState );									\
	}																								\
};

DECALRE_SIMPLE_HANDLER( CLEFTHandler, eHAlign, SState::HORALIGN_LEFT )
DECALRE_SIMPLE_HANDLER( CRIGHTHandler, eHAlign, SState::HORALIGN_RIGHT )
DECALRE_SIMPLE_HANDLER( CCENTERHandler, eHAlign, SState::HORALIGN_CENTER )
DECALRE_SIMPLE_HANDLER( CNOWRAPHandler, eHAlign, SState::HORALIGN_NOWRAP )
DECALRE_SIMPLE_HANDLER( CJUSTIFYHandler, eHAlign, SState::HORALIGN_JUSTIFY )
DECALRE_SIMPLE_HANDLER( CWRAPLEFTHandler, eHAlign, SState::HORALIGN_WRAP_LEFT )
DECALRE_SIMPLE_HANDLER( CWRAPRIGHTHandler, eHAlign, SState::HORALIGN_WRAP_RIGHT )
DECALRE_SIMPLE_HANDLER( CTOPHandler, eVAlign, SState::VERTALIGN_TOP )
DECALRE_SIMPLE_HANDLER( CMIDDLEHandler, eVAlign, SState::VERTALIGN_MIDDLE )
DECALRE_SIMPLE_HANDLER( CBOTTOMHandler, eVAlign, SState::VERTALIGN_BOTTOM )

// CBRHandler

class CBRHandler: public IMLHandler
{
	OBJECT_BASIC_METHODS( CBRHandler );
private:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

public:
	void Exec( IML *pIML, IMLLayout *pLayout, const std::vector<std::wstring> &paramsSet )
	{
		pLayout->AddCommand( CMD_BREAKLINE );
	}
};

// CCOLORHandler

struct SFadeValue: public CObjectBase
{
	OBJECT_BASIC_METHODS( SFadeValue )
		ZDATA
public:
	float fFadeValue;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&fFadeValue); return 0; }
	SFadeValue(): fFadeValue( 1.0f ){}
	SFadeValue( float _fFadeValue ): fFadeValue( _fFadeValue ){}
};
class CCOLORHandler: public IMLHandler
{
	OBJECT_BASIC_METHODS( CCOLORHandler );
private:
	ZDATA
	CPtr<SFadeValue> sFadeValue;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&sFadeValue); return 0; }

public:
	CCOLORHandler(){}
	CCOLORHandler( SFadeValue *pFadeValue ):sFadeValue( pFadeValue ){}
	void Exec( IML *pIML, IMLLayout *pLayout, const std::vector<std::wstring> &paramsSet )
	{
		if ( paramsSet.size() != 3 )
			return;

		SState sState = pLayout->GetState();
		sState.sColor = StringToColor( paramsSet[2], sFadeValue->fFadeValue );
		pLayout->SetState( sState );
	}
};

// CTABHandler

class CTABHandler: public IMLHandler
{
	OBJECT_BASIC_METHODS( CTABHandler );
private:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

public:
	void Exec( IML *pIML, IMLLayout *pLayout, const std::vector<std::wstring> &paramsSet )
	{
		pLayout->AddObject( CreateIMLTabObject() );
	}
};

// CFONTHandler

class CFONTHandler: public IMLHandler
{
	OBJECT_BASIC_METHODS( CFONTHandler );
private:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

public:
	void Exec( IML *pIML, IMLLayout *pLayout, const std::vector<std::wstring> &paramsSet )
	{
		SState sState = pLayout->GetState();
		if ( paramsSet.size() > 1 )
		{
			sState.sOutlineColor = sState.sColor;
			sState.nOutlineBorder = 0;

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
					wchar_t wsString[128];
					int nParams = swscanf( wsParam.c_str(), L"%d%2s", &sState.sFont.nSize, wsString );

					if ( nParams > 1 )
					{
						std::wstring wsSizeMod( wsString );
						if ( wsSizeMod.compare( L"px" ) == 0 )
							sState.sFont.nSize |= FONT_SIZE_PIXELS;
						else if ( wsSizeMod.compare( L"pt" ) == 0 )
							sState.sFont.nSize |= FONT_SIZE_POINTS;
					}
					else
						sState.sFont.nSize |= FONT_SIZE_POINTS;

				}
				else if ( wsID.compare( L"face" ) == 0  )
					NStr::UnicodeToUTF8( &sState.sFont.szName, wsParam );
				else if ( wsID.compare( L"outlinesize" ) == 0  )
					sState.nOutlineBorder = _wtol( wsParam.c_str() );
				else if ( wsID.compare( L"outlinecolor" ) == 0  )
					sState.sOutlineColor = StringToColor( wsParam );
				else if ( wsID.compare( L"forcefontsize" ) == 0  )
					sState.bForceFontSize = true;

				nTemp += 3;
			}
		}
		pLayout->SetState( sState );
	}
};

// CIMAGEHandler

class CIMAGEHandler: public IMLHandler
{
	OBJECT_BASIC_METHODS( CIMAGEHandler );
private:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

public:
	void Exec( IML *pIML, IMLLayout *pLayout, const std::vector<std::wstring> &paramsSet )
	{
		int nWidth = -1, nHeight = -1;
		int nBorder = 0;
		SState::EHORAlign eAlign = SState::HORALIGN_DEFAULT;
		CPtr<NDb::STexture> pTexture = 0;

		if ( paramsSet.size() > 1 )
		{
			int nTemp = 1;
			while( nTemp < paramsSet.size() )
			{
				if ( nTemp + 2 >= paramsSet.size() )
					break;
				if ( paramsSet[nTemp + 1].compare( L"=" ) != 0 )
					break;

				const std::wstring &wsID = paramsSet[nTemp];
				const std::wstring &wsParam = paramsSet[nTemp + 2];
				if ( wsID.compare( L"align" ) == 0  )
				{
					if ( wsParam.compare( L"left" ) == 0 )
						eAlign = SState::HORALIGN_WRAP_LEFT;
					else if ( wsParam.compare( L"right" ) == 0 )
						eAlign = SState::HORALIGN_WRAP_RIGHT;
				}
//				else if ( wsID.compare( L"id" ) == 0  )
//					pTexture = NDb::STexture::Get( _wtol( wsParam.c_str() ) );
				else if ( wsID.compare( L"width" ) == 0  )
					nWidth = _wtol( wsParam.c_str() );
				else if ( wsID.compare( L"height" ) == 0  )
					nHeight = _wtol( wsParam.c_str() );
				else if ( wsID.compare( L"border" ) == 0  )
					nBorder = _wtol( wsParam.c_str() );

				nTemp += 3;
			}
		}

		if ( IsValid( pTexture ) )
			pLayout->AddObject( CreateIMLImageObject( pTexture, eAlign, nBorder, nWidth, nHeight ) );
	}
};

// CMINFONTSIZEHandler

class CMINFONTSIZEHandler: public IMLHandler
{
	OBJECT_BASIC_METHODS(CMINFONTSIZEHandler);
private:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

public:
	void Exec( IML *pIML, IMLLayout *pLayout, const std::vector<std::wstring> &paramsSet )
	{
		SState sState = pLayout->GetState();
		if ( paramsSet.size() > 1 )
		{
			sState.sOutlineColor = sState.sColor;
			sState.nOutlineBorder = 0;

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
					sState.nMinFontSize = _wtol( wsParam.c_str() );

				nTemp += 3;
			}
		}
		pLayout->SetState( sState );
	}
};

// CStateStack

class CStateStack: public IMLHandler
{
	OBJECT_BASIC_METHODS(CStateStack);
private:
	ZDATA
	bool bSave;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&bSave); return 0; }

public:
	CStateStack() {}
	CStateStack( bool _bSave ): bSave( _bSave ) {}
	void Exec( IML *pIML, IMLLayout *pLayout, const std::vector<std::wstring> &paramsSet )
	{
		if ( bSave )
			pLayout->PushState();
		else
			pLayout->PopState();
	}
};


