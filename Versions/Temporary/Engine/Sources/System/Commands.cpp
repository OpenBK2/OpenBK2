#include "stdafx.h"

#include "Commands.h"
#include "Misc/StrProc.h"

namespace NGlobal
{

// Dependent command

CCmd::CCmd( const std::string &_szID, CmdHandler _pHandler, void *_pContext ):
	szID( _szID ), pHandler( _pHandler ), pContext( _pContext )
{
	nID = RegisterCmd( szID, pHandler, pContext );
}

CCmd::~CCmd()
{
	UnregisterCmd( szID, nID );
}

void CCmd::Run( const std::vector<std::wstring> &paramsSet )
{
	pHandler( szID, paramsSet, pContext );
}


void VarBoolHandler( const std::string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	bool *pFlag = (bool*)pContext;

	*pFlag = false;
	if ( sValue.GetFloat() == 1 )
		*pFlag = true;
}

void VarIntHandler( const std::string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	int *pValue = (int*)pContext;
	*pValue = Float2Int( sValue.GetFloat() );
}

void VarFloatHandler( const std::string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	float *pValue = (float*)pContext;
	*pValue = sValue.GetFloat();
}

void VarWStrHandler( const std::string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	std::wstring *pValue = (std::wstring*)pContext;
	*pValue = sValue.GetString();
}

void VarStrHandler( const std::string &szID, const NGlobal::CValue &sValue, void *pContext )
{
	std::string *pValue = (std::string*)pContext;
	*pValue = NStr::ToMBCS( sValue.GetString() );
}

}

