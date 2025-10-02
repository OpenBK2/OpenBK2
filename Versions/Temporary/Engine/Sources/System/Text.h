#pragma once

#include "System_export.h"

#define UNICODE_SIGNATURE 0xfeff

namespace NText
{
	//! get text from resources by filename
	SYSTEM_EXPORT const wstring &GetText( const string &szTextFileName );
	//! reload text to resources (discard cached information)
	void Reload( const string &szTextFileName );
	//! load unicode text from stream to wstring
	SYSTEM_EXPORT bool LoadUnicodeText( wstring *pwszRes, CDataStream *pStream );
}

// CRAP{ for transition-to-text-files period only
//#define CHECK_TEXT_NOT_EMPTY_PRE( pre_name, name ) ( pre_name##p##name != 0 || !pre_name##sz##name##FileRef.empty() )
//#define GET_TEXT_PRE( pre_name, name ) ( pre_name##sz##name##FileRef.empty() ? (pre_name##p##name == 0 ? NText::GetText("") : pre_name##p##name##->wszText ) : NText::GetText( pre_name##sz##name##FileRef ) )
//#define CHECK_TEXT_NOT_EMPTY( name ) ( p##name != 0 || !sz##name##FileRef.empty() )
//#define GET_TEXT( name ) ( sz##name##FileRef.empty() ? (p##name == 0 ? NText::GetText("") : p##name##->wszText ) : NText::GetText( sz##name##FileRef ) )
#define CHECK_TEXT_NOT_EMPTY_PRE( pre_name, name ) ( !pre_name##sz##name##FileRef.empty() )
#define GET_TEXT_PRE( pre_name, name ) NText::GetText( pre_name##sz##name##FileRef )
#define CHECK_TEXT_NOT_EMPTY( name ) ( !sz##name##FileRef.empty() )
#define GET_TEXT( name ) NText::GetText( sz##name##FileRef )
// CRAP}

