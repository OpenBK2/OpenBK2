#include "stdafx.h"
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

// CMLStream

void CMLStream::GetString( int nStart, int nSize, std::wstring *pRes )
{
	if ( nStart + nSize > wsText.length() )
	{
		ASSERT( 0 );
		return;
	}

	*pRes = wsText.substr( nStart, nSize );
}

void CMLStream::InsertString( const std::wstring &wsInsertText )
{
	std::wstring wsNewText( wsText.substr( 0, nPos ) + wsInsertText + wsText.substr( nPos ) );
	wsText = wsNewText;
	nSize = wsText.size();
}

// CML

class CML: public IML
{
	OBJECT_BASIC_METHODS(CML)
private:
	ZDATA
	std::wstring text;
	CTPoint<int> size;
	CObj<CMLStream> pStream;
	CObj<IReflowLayout> pLayout;
	std::unordered_map<std::wstring,CObj<IHandler> > tagsMap;
	CObj<SFadeValue> sFadeValue;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&text); f.Add(3,&size); f.Add(4,&pStream); f.Add(5,&pLayout); f.Add(6,&tagsMap); return 0; }

public:
	CML();

	void SetText( const std::wstring &_text ) { text = _text; }
	void SetHandler( const std::wstring &tag, IHandler *pHandler );
	void SetFade( float fFade, int nWidth );


	void Generate( NGScene::ILayoutFakeView *pView, int nWidth );

	void Render( std::list<CTRect<float> > *pRender, const CTPoint<float> &position, const CTRect<float> &window );
	void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &position, const CTRect<float> &window );

	const CTPoint<int>& GetSize() const { return size; }
};

CML::CML()
{
	sFadeValue = new SFadeValue( 1.0f );
	pLayout = CreateReflowLayout();

	tagsMap[L"br"] = new CBRHandler();
	tagsMap[L"tab"] = new CTabHandler();

	tagsMap[L"left"] = new CLEFTHandler;
	tagsMap[L"right"] = new CRIGHTHandler;
	tagsMap[L"center"] = new CCENTERHandler;
	tagsMap[L"nowrap"] = new CNOWRAPHandler;
	tagsMap[L"justify"] = new CJUSTIFYHandler;
	tagsMap[L"wrapleft"] = new CWRAPLEFTHandler;
	tagsMap[L"wrapright"] = new CWRAPRIGHTHandler;
	tagsMap[L"top"] = new CTOPHandler;
	tagsMap[L"bottom"] = new CBOTTOMHandler;
	tagsMap[L"middle"] = new CMIDDLEHandler;

	tagsMap[L"font"] = new CFontHandler();
	tagsMap[L"color"] = new CColorHandler( sFadeValue );
	tagsMap[L"minfontsize"] = new CMinFontSizeHandler();
}

void CML::SetHandler( const std::wstring &tag, IHandler *pHandler )
{
	tagsMap[tag] = pHandler;
}

void CML::SetFade( float fFade, int nWidth )
{
	if ( fFade != sFadeValue->fFadeValue )
		sFadeValue->fFadeValue = fFade;
}

void CML::Generate( NGScene::ILayoutFakeView *pView, int nWidth )
{
	enum ECharType
	{
		CHAR_NULL,
		CHAR_SPACE,
		CHAR_ALNUM,
		CHAR_PUNCTUATION,
		CHAR_EOL,
		////
		CHAR_TAG_END,
		CHAR_TAG_BEGIN
	};

	pStream = new CMLStream();
	pLayout = CreateReflowLayout();
	pStream->Seek( 0 );
	pStream->InsertString( text );

	int nWordBegin = 0;
	bool bTAG = false, bBracketsBlock = false;
	ECharType thisChar = CHAR_NULL, lastChar = CHAR_NULL;
	std::vector<std::wstring> paramsSet;

	for ( bool bContinue = true; bContinue; )
	{
		WCHAR wcChar = 0;
		if ( !pStream->IsEof() )
			wcChar = pStream->GetChar();
		else
			bContinue = false;
		thisChar = CHAR_NULL;
		if ( wcChar == '>' )
			thisChar = CHAR_TAG_END;
		else if ( wcChar == '<' )
			thisChar = CHAR_TAG_BEGIN;
		else if ( bTAG && ( wcChar == '\"' ) )
			bBracketsBlock = !bBracketsBlock;
		else if ( iswalnum( wcChar ) != 0 )
			thisChar = CHAR_ALNUM;
		else if ( iswpunct( wcChar ) )
			thisChar = bTAG ? CHAR_PUNCTUATION : CHAR_ALNUM; //// CRAP: Tag need separate '='
		else if  ( !bBracketsBlock && ( wcChar == L' ' ) )
			thisChar = CHAR_SPACE;
		else if  ( bBracketsBlock && ( wcChar == L' ' ) )
			thisChar = lastChar;
		else if ( ( wcChar == L'\0' ) || ( wcChar == L'\n' ) )
			thisChar = CHAR_EOL;

		if ( lastChar != thisChar )
		{
			if ( !bTAG && ( ( lastChar == CHAR_ALNUM ) || ( lastChar == CHAR_PUNCTUATION ) ) )
				pLayout->AddObject( CreateTextObject( pStream, nWordBegin, pStream->GetSeek() - nWordBegin ) );
			else if ( !bTAG && ( lastChar == CHAR_SPACE ) )
				pLayout->AddObject( CreateSpringObject( pStream->GetSeek() - nWordBegin ) );
			else if ( bTAG && ( ( lastChar == CHAR_ALNUM ) || ( lastChar == CHAR_PUNCTUATION ) ) )
			{
				std::wstring &wsTemp = paramsSet.emplace_back();
				pStream->GetString( nWordBegin, pStream->GetSeek() - nWordBegin, &wsTemp );
			}

			nWordBegin = pStream->GetSeek();
		}

		pStream->IncSeek();
		lastChar = thisChar;

		if ( lastChar == CHAR_TAG_END )
		{
			bTAG = false;

			if ( !paramsSet.empty() )
			{
				std::unordered_map<std::wstring,CObj<IHandler> >::const_iterator iTemp = tagsMap.find( paramsSet.front() );
				if ( ( iTemp != tagsMap.end() ) && ( iTemp->second != 0 ) )
					iTemp->second->Exec( pStream, pLayout, paramsSet );
			}
		}
		else if ( lastChar == CHAR_TAG_BEGIN )
		{
			bTAG = true;
			paramsSet.clear();
			paramsSet.reserve( 8 );
		}
	};

	pLayout->Generate( pView, nWidth );
	size.x = Float2Int( pLayout->GetSize().x );
	size.y = Float2Int( pLayout->GetSize().y );
}

void CML::Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &position, const CTRect<float> &window )
{
	pLayout->Render( pView, position, window );
}

void CML::Render( std::list<CTRect<float> > *pRender, const CTPoint<float> &position, const CTRect<float> &window )
{
	pLayout->Render( pRender, position, window );
}

// CreateML

IML* CreateML()
{
	return new CML;
}

} // NAMESPACE

using namespace NML;
REGISTER_SAVELOAD_CLASS( 0xB5529160, CML )
REGISTER_SAVELOAD_CLASS( 0xB5529161, CMLStream )

