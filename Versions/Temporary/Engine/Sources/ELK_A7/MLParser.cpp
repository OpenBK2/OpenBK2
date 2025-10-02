#include "StdAfx.h"
#include "TranslateEdit.h"
#include "WMDefines.h"

#include "MLParser.h"
#include "..\MapEditorLib\Tools_Resources.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace NML
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int Parse( CMLUnicodeText *pMLUnicodeText, const wstring &rwszText, bool bJoinTags )
	{
		int nPartAdded = 0;
		if ( pMLUnicodeText == 0 )
		{
			return nPartAdded;
		}
		int i = rwszText.find( wchar_t( '<' ), 0 );
		if ( i == wstring::npos )
		{
			++nPartAdded;
			CMLUnicodeText::iterator itMLUnicodeTextPart = pMLUnicodeText->insert( pMLUnicodeText->end(), SMLUnicodeTextPart() );
			itMLUnicodeTextPart->wszText = rwszText;
			return nPartAdded;
		}
		else if ( i > 0 )
		{
			++i;
			++nPartAdded;
			CMLUnicodeText::iterator itMLUnicodeTextPart = pMLUnicodeText->insert( pMLUnicodeText->end(), SMLUnicodeTextPart() );
			itMLUnicodeTextPart->wszText = rwszText.substr( 0, i );
		}
		//
		while ( i < rwszText.size() )
		{
			int nTagEnd = rwszText.find( wchar_t( '>' ), i );
			int nTextEnd = ( ( i + 1 ) >= rwszText.size() ) ? wstring::npos : rwszText.find( wchar_t( '<' ), i + 1 ) - 1;

			if ( ( nTagEnd >= 0 ) && ( nTextEnd >= 0 ) && ( nTextEnd < nTagEnd ) )
			{
				nTextEnd = nTagEnd + 1;
			}

			wstring wszTag = ( nTagEnd >= 0 ) ? rwszText.substr( i, 1 + nTagEnd - i ) : rwszText.substr( i );
			wstring wszText;
			if ( nTagEnd >= 0 )
			{
				wszText = ( nTextEnd >= 0 ) ? rwszText.substr( nTagEnd + 1, nTextEnd - nTagEnd ) : rwszText.substr( nTagEnd + 1 );
			}
			//
			if ( bJoinTags && ( nPartAdded > 0 ) )
			{
				SMLUnicodeTextPart& rMLUnicodeTextPart = ( *pMLUnicodeText )[pMLUnicodeText->size() - 1];
				if ( rMLUnicodeTextPart.wszText.empty() )
				{
					rMLUnicodeTextPart.wszTag += wszTag;
					rMLUnicodeTextPart.wszText = wszText;
				}
				else
				{
					++nPartAdded;
					CMLUnicodeText::iterator itMLUnicodeTextPart = pMLUnicodeText->insert( pMLUnicodeText->end(), SMLUnicodeTextPart() );
					itMLUnicodeTextPart->wszTag = wszTag;
					itMLUnicodeTextPart->wszText = wszText;
				}
			}
			else
			{
				++nPartAdded;
				CMLUnicodeText::iterator itMLUnicodeTextPart = pMLUnicodeText->insert( pMLUnicodeText->end(), SMLUnicodeTextPart() );
				itMLUnicodeTextPart->wszTag = wszTag;
				itMLUnicodeTextPart->wszText = wszText;
			}
			//
			if ( nTextEnd < 0 )
			{
				break;
			}
			i = nTextEnd + 1;
		}
		return nPartAdded;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int Parse( CMLMBCSText *pMLMBCSText, const CString &rstrText, bool bJoinTags, int nCodePage )
	{
		int nPartAdded = 0;
		if ( pMLMBCSText )
		{
			wstring wszText;
			MBSC2Unicode( &wszText, rstrText, nCodePage );
			CMLUnicodeText mlUnicodeText;
			Parse( &mlUnicodeText, wszText, bJoinTags );
			for ( CMLUnicodeText::const_iterator itMLUnicodeTextPart = mlUnicodeText.begin(); itMLUnicodeTextPart != mlUnicodeText.end(); ++itMLUnicodeTextPart )
			{
				CMLMBCSText::iterator itCMLMBCSTextPart = pMLMBCSText->insert( pMLMBCSText->end(), SMLMBCSTextPart() );
				if ( !itMLUnicodeTextPart->wszTag.empty() )
				{
					Unicode2MBSC( &( itCMLMBCSTextPart->strTag ), itMLUnicodeTextPart->wszTag, nCodePage );
				}
				if ( !itMLUnicodeTextPart->wszText.empty() )
				{
					Unicode2MBSC( &( itCMLMBCSTextPart->strText ), itMLUnicodeTextPart->wszText, nCodePage );
				}
				++nPartAdded;
			}
		}
		return nPartAdded;
	}
};


