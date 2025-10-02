#include "StdAfx.h"

#include "Commands.h"
#include "Misc/StrProc.h"

namespace NGlobal
{

// Dependent command

CCmd::CCmd( const string &_szID, CmdHandler _pHandler, void *_pContext ):
	szID( _szID ), pHandler( _pHandler ), pContext( _pContext )
{
	nID = RegisterCmd( szID, pHandler, pContext );
}

CCmd::~CCmd()
{
	UnregisterCmd( szID, nID );
}

void CCmd::Run( const vector<wstring> &paramsSet )
{
	pHandler( szID, paramsSet, pContext );
}


void VarBoolHandler( const string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	bool *pFlag = (bool*)pContext;

	*pFlag = false;
	if ( sValue.GetFloat() == 1 )
		*pFlag = true;
}

void VarIntHandler( const string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	int *pValue = (int*)pContext;
	*pValue = Float2Int( sValue.GetFloat() );
}

void VarFloatHandler( const string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	float *pValue = (float*)pContext;
	*pValue = sValue.GetFloat();
}

void VarWStrHandler( const string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	wstring *pValue = (wstring*)pContext;
	*pValue = sValue.GetString();
}

void VarStrHandler( const string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	string *pValue = (string*)pContext;
	*pValue = NStr::ToMBCS( sValue.GetString() );
}

}

