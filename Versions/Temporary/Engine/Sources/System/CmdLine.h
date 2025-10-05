#pragma once

namespace NCmdLine
{

// dbcodegen -all --dry-run --types-path "c:\b2\data\" --config "dbconfig.xml"
// cl.AddOption( "all", cl.CreateObserver(&eType, MYVAL_ALL), "generate types.xml and sources" );

class CCmdLine
{
public:
	struct IObserver;
	enum EProcessResult
	{
		PROC_RESULT_OK,										// command line processed successfully
		PROC_RESULT_AMBIGUITY,						// processing failed - ambiguity parameter
		PROC_RESULT_VALUE_NOT_FOUND,			// value for option not found in command line
		PROC_RESULT_NO_ARGUMENTS,					// arguments required, but doesn't provided
	};
private:
	template <class TYPE> class CObserver;
	struct SObserver
	{
		std::string szName;
		std::string szDescription;
		CObj<IObserver> pObserver;
	};
	typedef std::list<SObserver> CObserversList;
	CObserversList observers;
	std::string szHeader;
	//
	const SObserver *Find( const char *pszName ) const;
	bool AddOptionInternal( const char *pszName, IObserver *pObserver, const char *pszDescription );
	// observer with external value
	IObserver *MakeIntObserver( int *pRes ) const;
	IObserver *MakeFloatObserver( float *pRes ) const;
	IObserver *MakeStringObserver( std::string *pRes ) const;
	IObserver *MakeWStringObserver( std::wstring *pRes ) const;
	// observer with internal value
	IObserver *MakeIntObserver( int *pRes, const int &setval ) const;
	IObserver *MakeFloatObserver( float *pRes, const float &setval ) const;
	IObserver *MakeStringObserver( std::string *pRes, const std::string &setval ) const;
	IObserver *MakeWStringObserver( std::wstring *pRes, const std::wstring &setval ) const;
public:
	CCmdLine( const std::string &_szHeader ): szHeader( _szHeader ) {}
	// add option with internal value
	// common case for enums only!
	template <class TVal>
		bool AddOption( const char *pszName, TVal *pRes, const TVal &setval, const char *pszDescription )
	{
		assert( sizeof(TVal) == 4 );
		return AddOption<int>( pszName, (int*)pRes, *((int*)&setval), pszDescription );
	}
	template <>
		bool AddOption<int>( const char *pszName, int *pRes, const int &setval, const char *pszDescription )
	{
		return AddOptionInternal( pszName, MakeIntObserver(pRes, setval), pszDescription );
	}
	template <>
		bool AddOption<float>( const char *pszName, float *pRes, const float &setval, const char *pszDescription )
	{
		return AddOptionInternal( pszName, MakeFloatObserver(pRes, setval), pszDescription );
	}
	template <>
		bool AddOption<std::string>( const char *pszName, std::string *pRes, const std::string &setval, const char *pszDescription )
	{
		return AddOptionInternal( pszName, MakeStringObserver(pRes, setval), pszDescription );
	}
	template <>
		bool AddOption<std::wstring>( const char *pszName, std::wstring *pRes, const std::wstring &setval, const char *pszDescription )
	{
		return AddOptionInternal( pszName, MakeWStringObserver(pRes, setval), pszDescription );
	}
	// add option with external value
	// common case for enums only!
	template <class TVal>
		bool AddOption( const char *pszName, TVal *pRes, const char *pszDescription )
	{
		assert( sizeof(TVal) == 4 );
		return AddOption<int>( pszName, (int*)pRes, pszDescription );
	}
	template <>
		bool AddOption<int>( const char *pszName, int *pRes, const char *pszDescription )
	{
		return AddOptionInternal( pszName, MakeIntObserver(pRes), pszDescription );
	}
	template <>
		bool AddOption<float>( const char *pszName, float *pRes, const char *pszDescription )
	{
		return AddOptionInternal( pszName, MakeFloatObserver(pRes), pszDescription );
	}
	template <>
		bool AddOption<std::string>( const char *pszName, std::string *pRes, const char *pszDescription )
	{
		return AddOptionInternal( pszName, MakeStringObserver(pRes), pszDescription );
	}
	template <>
		bool AddOption<std::wstring>( const char *pszName, std::wstring *pRes, const char *pszDescription )
	{
		return AddOptionInternal( pszName, MakeWStringObserver(pRes), pszDescription );
	}
	//! process command line
	EProcessResult Process( const std::vector<std::string> &args ) const;
	EProcessResult Process( const char *pszCommandLine ) const;
	EProcessResult Process( int argc, char *argv[] ) const;
	//! print usage help
	int PrintUsage( const char *pszAdd ) const; // always return 0xDEAD
	//! print copyright info
	int PrintHeader() const;
	//! get help string for option 'pszName'
	std::string GetHelp( const char *pszName ) const;
};

}
