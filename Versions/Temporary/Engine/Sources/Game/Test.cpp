#include "StdAfx.h"

#include "../Script/Script.h"
#include "../System/Commands.h"
#include "../System/MetaProg.h"
#include "../Misc/StrProc.h"


// ************************************************************************************************************************ //
// **
// ** 
// **
// **
// **
// ************************************************************************************************************************ //

namespace NGlobal
{

interface IConsoleFunc : public CObjectBase
{
	virtual bool Execute( const string &szID, const vector<wstring> &paramsSet ) = 0;
};

void ConvertParam( bool *pRes, const wstring &wszParam )
{
	*pRes = !( wszParam[0] == L'0' || wszParam[0] == L'f' || wszParam[0] == L'F' );
}
void ConvertParam( bool *pRes, const CValue &val )
{
	*pRes = val.GetFloat() != 0;
}

void ConvertParam( int *pRes, const wstring &wszParam )
{
	*pRes = NStr::ToInt( NStr::ToMBCS( wszParam ) );
}
void ConvertParam( int *pRes, const CValue &val )
{
	*pRes = val.GetFloat();
}
void ConvertParam( float *pRes, const wstring &wszParam )
{
	*pRes = NStr::ToFloat( NStr::ToMBCS( wszParam ) );
}
void ConvertParam( string *pRes, const wstring &wszParam )
{
	*pRes = NStr::ToMBCS( wszParam );
}
void ConvertParam( wstring *pRes, const wstring &wszParam )
{
	*pRes = wszParam;
}

#define MAKE_PARAMETER( num, nNumParams )				\
	TS##num p##num;																\
	if ( nNumParams > num - 1 )										\
		ConvertParam( &p##num, paramsSet[num - 1] );\
	else																					\
		p##num = def##num;

class CConsoleFunc0 : public IConsoleFunc
{
	OBJECT_NOCOPY_METHODS( CConsoleFunc0 );
	void (*pfnFunc)();
	//
	CConsoleFunc0() {}
public:
	CConsoleFunc0( void (*_pfnFunc)() )
		: pfnFunc( _pfnFunc ) {}
	//
	bool Execute( const string &szID, const vector<wstring> &paramsSet )
	{
		const int nNumParams = paramsSet.size();
		(*pfnFunc)();
		return true;
	}
};

//IConsoleFunc *MakeConsoleFunc( void (*_pfnFunc)() )
//{
//	return new CConsoleFunc0( _pfnFunc );
//}

template <typename T1>
class CConsoleFunc1 : public IConsoleFunc
{
	OBJECT_NOCOPY_METHODS( CConsoleFunc1 );
	void (*pfnFunc)( T1 p1 );
	typedef typename NMeta::CTypeTraits<T1>::TStripType TS1;
	TS1 def1;
	//
	CConsoleFunc1() {}
public:
	CConsoleFunc1( void (*_pfnFunc)( T1 _p1 ), const TS1 &_def1 )
		: pfnFunc( _pfnFunc ), def1( _def1 ) {}
	//
	bool Execute( const string &szID, const vector<wstring> &paramsSet )
	{
		const int nNumParams = paramsSet.size();
		MAKE_PARAMETER( 1, nNumParams );
		(*pfnFunc)( p1 );
		return true;
	}
};

template <typename T1>
IConsoleFunc *MakeConsoleFunc( void (*_pfnFunc)( T1 _p1 ), T1 _p1 )
{
	return new CConsoleFunc1( _pfnFunc, _p1 );
}

template <typename T1, typename T2>
class CConsoleFunc2 : public IConsoleFunc
{
	OBJECT_NOCOPY_METHODS( CConsoleFunc2 );
	void (*pfnFunc)( T1 p1, T2 p2 );
	typedef typename NMeta::CTypeTraits<T1>::TStripType TS1;
	typedef typename NMeta::CTypeTraits<T2>::TStripType TS2;
	TS1 def1;
	TS2 def2;
	//
	CConsoleFunc2() {}
public:
	CConsoleFunc2( void (*_pfnFunc)( T1 _p1, T2 _p2 ), const TS1 &_def1, const TS2 &_def2 )
		: pfnFunc( _pfnFunc ), def1( _def1 ), def2( _def2 ) {}
	//
	bool Execute( const string &szID, const vector<wstring> &paramsSet )
	{
		const int nNumParams = paramsSet.size();
		MAKE_PARAMETER( 1, nNumParams );
		MAKE_PARAMETER( 2, nNumParams );
		(*pfnFunc)( p1, p2 );
		return true;
	}
};

template <typename T1, typename T2>
IConsoleFunc *MakeConsoleFunc( void (*_pfnFunc)( T1 _p1, T2 _p2 ), T1 _p1, T2 _p2 )
{
	return new CConsoleFunc2( _pfnFunc, _p1, _p2 );
}

}


