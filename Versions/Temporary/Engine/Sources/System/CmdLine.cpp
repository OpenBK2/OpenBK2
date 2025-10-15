#include "stdafx.h"

#include "CmdLine.h"
#include "Misc/StrProc.h"

#include "System_export.h"

namespace NCmdLine
{

static void Report( const char *pszFormat, ... )
{
	static const int BUF_SIZE = 4096;
	static char charBuff[BUF_SIZE];
	va_list va;
	va_start( va, pszFormat );
	_vsnprintf( charBuff, BUF_SIZE - 1, pszFormat, va );
	va_end( va );
	//
	printf( charBuff );
	OutputDebugString( charBuff );
}

static std::string GetErrorDesc( CCmdLine::EProcessResult eResult )
{
	switch ( eResult )
	{
	case CCmdLine::PROC_RESULT_OK:
		return "Ok";
	case CCmdLine::PROC_RESULT_NO_ARGUMENTS:
		return "No arguments found, but required";
	case CCmdLine::PROC_RESULT_AMBIGUITY:
		return "Ambiguous param value";
	case CCmdLine::PROC_RESULT_VALUE_NOT_FOUND:
		return "No value found";
	}
	return "Unknown error";
}

// ************************************************************************************************************************ //
// **
// ** observers
// **
// **
// **
// ************************************************************************************************************************ //

struct CCmdLine::IObserver: public CObjectBase
{
	virtual bool IsNeedValue() const = 0;
	virtual CCmdLine::EProcessResult AcceptValue( const std::string &value ) = 0;
};

template <class TYPE>
class CCmdLine::CObserver : public CCmdLine::IObserver
{
	OBJECT_BASIC_METHODS( CObserver );
	//
	const bool bNeedValue;
	const TYPE tInternal;
	const TYPE tInitialValue;	// store initial value of *pRes here to compare then setting new!
	TYPE *pResult;
	//
	template <class TRes> TRes Convert( const std::string &input ) { return Convert<int>(input); }
	template <> int Convert<int>( const std::string &input ) { return NStr::ToInt(input); }
	template <> float Convert<float>( const std::string &input ) { return NStr::ToFloat(input); }
	template <> std::string Convert<std::string>( const std::string &input ) { return input; }
	template <> std::wstring Convert<std::wstring>( const std::string &input ) { return NStr::ToUnicode(input); }
public:
	CObserver(): bNeedValue( false ), tInternal( TYPE() ), tInitialValue( TYPE() ), pResult( 0 ) {}
	CObserver( TYPE *_pResult ): bNeedValue( true ), tInternal( TYPE() ), tInitialValue( *_pResult ), pResult( _pResult ) {}
	CObserver( TYPE *_pResult, const TYPE &tSetVal ): bNeedValue( false ), tInternal( tSetVal ), tInitialValue( *_pResult ), pResult( _pResult ) {}
	//
	bool IsNeedValue() const { return bNeedValue; }
	CCmdLine::EProcessResult AcceptValue( const std::string &value )
	{ 
		if ( *pResult != tInitialValue )
			return CCmdLine::PROC_RESULT_AMBIGUITY;
		*pResult = bNeedValue ? Convert<TYPE>( value ) : tInternal; 
		return CCmdLine::PROC_RESULT_OK;
	}
};

// ************************************************************************************************************************ //
// **
// ** processing functions
// **
// **
// **
// ************************************************************************************************************************ //

const CCmdLine::SObserver *CCmdLine::Find( const char *pszName ) const
{
	for ( CObserversList::const_iterator it = observers.begin(); it != observers.end(); ++it )
	{
		if ( it->szName == pszName )
			return &( *it );
	}
	return 0;
}

std::string CCmdLine::GetHelp( const char *pszName ) const
{
	if ( const SObserver *pObserver = Find( pszName ) )
		return pObserver->szDescription;
	else
		return "";
}

bool CCmdLine::AddOptionInternal( const char *pszOptionName, IObserver *_pObserver, const char *pszDescription )
{
	CPtr<IObserver> pObserver = _pObserver;
	if ( pszOptionName == 0 || pObserver == 0 || Find(pszOptionName) != 0 )
		return false;
	//
	CObserversList::iterator pos = observers.insert( observers.end(), SObserver() );
	pos->szName = pszOptionName;
	pos->szDescription = ( pszDescription == 0 ? "" : pszDescription );
	pos->pObserver = pObserver;
	return true;
}

CCmdLine::IObserver *CCmdLine::MakeIntObserver( int *pRes ) const { return new CObserver<int>( pRes ); }
CCmdLine::IObserver *CCmdLine::MakeFloatObserver( float *pRes ) const { return new CObserver<float>( pRes ); }
CCmdLine::IObserver *CCmdLine::MakeStringObserver( std::string *pRes ) const { return new CObserver<std::string>( pRes ); }
CCmdLine::IObserver *CCmdLine::MakeWStringObserver( std::wstring *pRes ) const {	return new CObserver<std::wstring>( pRes ); }
CCmdLine::IObserver *CCmdLine::MakeIntObserver( int *pRes, const int &setval ) const { return new CObserver<int>( pRes, setval ); }
CCmdLine::IObserver *CCmdLine::MakeFloatObserver( float *pRes, const float &setval ) const { return new CObserver<float>( pRes, setval ); }
CCmdLine::IObserver *CCmdLine::MakeStringObserver( std::string *pRes, const std::string &setval ) const { return new CObserver<std::string>( pRes, setval ); }
CCmdLine::IObserver *CCmdLine::MakeWStringObserver( std::wstring *pRes, const std::wstring &setval ) const { return new CObserver<std::wstring>( pRes, setval ); }

int CCmdLine::PrintUsage( const char *pszAdd ) const
{
	std::string szMessage = szHeader + "\n";
	if ( pszAdd != 0 )
		szMessage += std::string( pszAdd ) + "\n";
	//
	for ( CObserversList::const_iterator it = observers.begin(); it != observers.end(); ++it )
		szMessage += "\t" + it->szName + " - " + it->szDescription + "\n";
	//
	Report( szMessage.c_str() );
	return 0xDEAD;
}

int CCmdLine::PrintHeader() const
{
	Report( (szHeader + "\n").c_str() );
	return 0xDEAD;
}

CCmdLine::EProcessResult CCmdLine::Process( const char *pszCommandLine ) const
{
	std::vector<std::string> strings;
	strings.reserve( 16 );
	// split input line
	for ( NStr::CStringIterator<char, std::string, NStr::CBracketSeparator<char, NStr::SQuoteTest<char> > > it( pszCommandLine, ' ' ); !it.IsEnd(); it.Next() )
	{
		std::string szVal;
		it.Get( &szVal );
		if ( !szVal.empty() )
		{
			NStr::TrimBoth( szVal, '\"' );
			strings.push_back( szVal );
		}
	}
	return Process( strings );
}

CCmdLine::EProcessResult CCmdLine::Process( int argc, char *argv[] ) const
{
	std::vector<std::string> strings;
	strings.reserve( 16 );
	for ( int i = 1; i < argc; ++i )
	{
		std::string szVal = argv[i];
		if ( !szVal.empty() )
		{
			NStr::TrimBoth( szVal, '\"' );
			strings.push_back( szVal );
		}
	}
	//
	return Process( strings );
}
CCmdLine::EProcessResult CCmdLine::Process( const std::vector<std::string> &args ) const
{
	if ( args.empty() )
	{
		if ( observers.empty() )
			return CCmdLine::PROC_RESULT_OK;
		else
		{
			Report( (GetErrorDesc(CCmdLine::PROC_RESULT_NO_ARGUMENTS) + "\n").c_str() );
			return CCmdLine::PROC_RESULT_NO_ARGUMENTS;
		}
	}
	//
	for ( int i = 0; i < args.size(); ++i )
	{
		if ( const SObserver *pObserver = Find( args[i].c_str() ) )
		{
			if ( pObserver->pObserver->IsNeedValue() )
			{
				if ( i + 1 < args.size() )
				{
					const EProcessResult eResult = pObserver->pObserver->AcceptValue( args[++i] );
					if ( eResult != PROC_RESULT_OK )
					{
						Report( (GetErrorDesc(eResult) + ": " + pObserver->szName + "\n").c_str() );
						return eResult;
					}
				}
				else
				{
					Report( (GetErrorDesc(PROC_RESULT_VALUE_NOT_FOUND) + ": " + pObserver->szName + "\n").c_str() );
					return PROC_RESULT_VALUE_NOT_FOUND;
				}
			}
			else
			{
				const EProcessResult eResult = pObserver->pObserver->AcceptValue( args[i] );
				if ( eResult != PROC_RESULT_OK )
				{
					Report( (GetErrorDesc(eResult) + ": " + pObserver->szName + "\n").c_str() );
					return eResult;
				}
			}
		}
	}
	return PROC_RESULT_OK;
}

}

BASIC_REGISTER_CLASS( SYSTEM, NCmdLine::CCmdLine::IObserver );
/*
using namespace NCmdLine;
namespace NTest
{
enum ECmdLineMode
{
	MODE_UNKNOWN,
	MODE_ALL,
	MODE_NOT_ALL,
};
struct SCmdLineTest
{
	SCmdLineTest()
	{
		int nInternal = -1, nExternal = -2;
		float fInternal = -1.1f, fExternal = -2.2f;
		string szInternal = "def-int", szExternal = "def-ext";
		wstring wszInternal = L"w-def-int", wszExternal = L"w-def-ext";
		ECmdLineMode eMode = MODE_UNKNOWN;
		CCmdLine cmdLine( "Command Line test" );
		cmdLine.AddOption( "-int-internal", &nInternal, 1, "test for internal int" );
		cmdLine.AddOption( "-int-external", &nExternal, "test for external int" );
		cmdLine.AddOption( "-float-internal", &fInternal, 1.1f, "test for internal float" );
		cmdLine.AddOption( "-float-external", &fExternal, "test for external float" );
		cmdLine.AddOption( "-string-internal", &szInternal, string("set"), "test for internal string" );
		cmdLine.AddOption( "-string-external", &szExternal, "test for external string" );
		cmdLine.AddOption( "-wstring-internal", &wszInternal, wstring(L"w-set"), "test for internal wstring" );
		cmdLine.AddOption( "-wstring-external", &wszExternal, "test for external wstring" );
		cmdLine.AddOption( "-enum", &eMode, MODE_UNKNOWN, MODE_ALL, "test for internal enum" );
		cmdLine.Process( "-int-internal -float-internal -float-external 3.14 -string-external \"la-la-la with space\" -wstring-internal -enum" );
		cmdLine.PrintUsage( "Usage: cmdline <options>" );
	}
};
static SCmdLineTest aCmdLineTest;
}
*/
