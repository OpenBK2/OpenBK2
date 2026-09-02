#include "stdafx.h"
#include <fmt/format.h>

#include "BuilderMethods.h"

#include "MapEditorLib/ManipulatorManager.h"

bool CheckStringValue( std::string *pszDescription, const std::string &szValueName, IManipulator *pBuilderMan )
{
	std::string szValue;
	if ( !CManipulatorManager::GetValue( &szValue, pBuilderMan, szValueName ) || szValue.empty() )
	{
		( *pszDescription ) = fmt::format( "<{}> must be filled.", szValueName.c_str() );
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
		( *pszDescription ) = fmt::format( "<{}> must be in range ({}...{}).", szValueName.c_str(), nMin, nMax );
		return false;
	}
	pszDescription->clear();
	return true;
}

// basement storage  


