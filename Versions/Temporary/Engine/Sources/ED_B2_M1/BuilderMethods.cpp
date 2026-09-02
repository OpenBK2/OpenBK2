#include "stdafx.h"

#include "BuilderMethods.h"

#include "MapEditorLib/ManipulatorManager.h"

bool CheckStringValue( std::string *pszDescription, const std::string &szValueName, IManipulator *pBuilderMan )
{
	std::string szValue;
	if ( !CManipulatorManager::GetValue( &szValue, pBuilderMan, szValueName ) || szValue.empty() )
	{
		( *pszDescription ) = StrFmt( "<%s> must be filled.", szValueName.c_str() );
		return false;
	}
	pszDescription->clear();
	return true;
}

bool CheckIntValue( std::string *pszDescription, const std::string &szValueName, int nMin, int nMax, IManipulator *pBuilderMan )
{
	int nValue = 0;
	if ( !CManipulatorManager::GetValue( &nValue, pBuilderMan, szValueName ) || ( nValue < nMin ) || ( nValue > nMax ) )
	{
		( *pszDescription ) = StrFmt( "<%s> must be in range (%d...%d).", szValueName.c_str(), nMin, nMax );
		return false;
	}
	pszDescription->clear();
	return true;
}

// basement storage  


