#include "stdafx.h"
#include <fmt/format.h>

#include "StringManager.h"
#include "Misc/StrProc.h"
#include "libdb/EditorDb.h"
#include "Interface_Controller.h"
#include "Interface_UserData.h"
#include "libdb/ResourceManager.h"
#include "Tools_HashSet.h"

void CStringManager::CreateRecentListName( std::string *pszName, const SObjectSet &rObjectSet, bool bMainObject )
{
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	//
	if ( pszName == 0 )
	{
		return;
	}
	if ( rObjectSet.objectNameSet.empty() )
	{
		return;
	}
	pszName->clear();
	const std::string szObjectName = rObjectSet.objectNameSet.begin()->first.ToString();
	if ( !szObjectName.empty() )
	{
		if ( bMainObject )
		{
			( *pszName ) = szObjectName;
		}
		else
		{
			CStringManager::GetRefValueFromTypeAndName( pszName, rObjectSet.szObjectTypeName, szObjectName, TYPE_SEPARATOR_CHAR );
		}
	}
}


void CStringManager::CreateObjectSet( SObjectSet *pObjectSet, const std::string &rszName, bool bMainObject )
{
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	if ( pObjectSet == 0 )
	{
		return;
	}
	pObjectSet->szObjectTypeName.clear();
	pObjectSet->objectNameSet.clear();
	std::string szObjectTypeName;
	std::string szObjectName;
	if ( bMainObject )
	{
		szObjectTypeName = pUserData->constUserData.szMainObjectType;
		szObjectName = rszName;
	}
	else
	{
		CStringManager::GetTypeAndNameFromRefValue( &szObjectTypeName, &szObjectName, rszName, TYPE_SEPARATOR_CHAR, pUserData->constUserData.szMainObjectType );
	}
	pObjectSet->szObjectTypeName = szObjectTypeName;	
	InsertHashSetElement( &( pObjectSet->objectNameSet ), CDBID( szObjectName ) );
}


void CStringManager::AddToRecentList( const std::string &rszName, bool bMainObject )
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	RemoveFromRecentList( rszName, bMainObject );
	//
	if ( bMainObject )
	{
		pUserData->recentList.push_front( rszName );
		if ( pUserData->recentList.size() > 10 )
		{
			pUserData->recentList.pop_back();
		}
	}
	else
	{
		pUserData->recentResourceList.push_front( rszName );
		if ( pUserData->recentResourceList.size() > 10 )
		{
			pUserData->recentResourceList.pop_back();
		}
	}
}


void CStringManager::RemoveFromRecentList( const std::string &rszName, bool bMainObject )
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	std::string szName = rszName;
	NStr::ToLower( &szName );
	if ( bMainObject )
	{
		for ( SUserData::CRecentList::iterator itRecentName = pUserData->recentList.begin(); itRecentName != pUserData->recentList.end(); )
		{
			std::string szRecentName = ( *itRecentName );
			NStr::ToLower( &szRecentName );
			if ( szRecentName == szName )
			{
				itRecentName = pUserData->recentList.erase( itRecentName );
			}
			else
			{
				++itRecentName;
			}
		}
	}
	else
	{
		for ( SUserData::CRecentList::iterator itRecentName = pUserData->recentResourceList.begin(); itRecentName != pUserData->recentResourceList.end(); )
		{
			std::string szRecentName = ( *itRecentName );
			NStr::ToLower( &szRecentName );
			if ( szRecentName == szName )
			{
				itRecentName = pUserData->recentResourceList.erase( itRecentName );
			}
			else
			{
				++itRecentName;
			}
		}
	}
}


bool CStringManager::GetStringValueFromString( const std::string &rszString, const std::string &rszLabel, const int nPos, const std::string &rszDividers, const std::string &rszDefaultValue, std::string *pszString )
{
	NI_ASSERT( pszString != 0, "CStringManager::GetStringValueFromString(): pszString == 0" );
	//
	if ( pszString )
	{
		( *pszString ) = rszDefaultValue;
		const int nStringPos = rszString.find( rszLabel, nPos );
		if ( nStringPos == std::string::npos )
		{
			return false;
		}
		const int nLeftPos = rszString.find_first_not_of( rszDividers, nStringPos + rszLabel.size() );
		if ( nLeftPos == std::string::npos )
		{
			return false;
		}
		if ( rszString[nLeftPos] == '"' )
		{
			const int nRightPos = rszString.find_first_of( '"', nLeftPos + 1 );
			( *pszString ) = rszString.substr( nLeftPos + 1, nRightPos - nLeftPos - 1 );
		}
		else
		{
			const int nRightPos = rszString.find_first_of( rszDividers, nLeftPos );
			( *pszString ) = rszString.substr( nLeftPos, nRightPos - nLeftPos );
		}
		return true;
	}
	return false;
}


int CStringManager::GetIntValueFromString( const std::string &rszString, const std::string &rszLabel, const int nPos, const std::string &rszDividers, int nDefaultValue )
{
	const int nStringPos = rszString.find( rszLabel, nPos );
	if ( nStringPos == std::string::npos )
	{
		return nDefaultValue;
	}
	const int nLeftPos = rszString.find_first_not_of( rszDividers, nStringPos + rszLabel.size() );
	if ( nLeftPos == std::string::npos )
	{
		return nDefaultValue;
	}
	std::string szString;
	if ( rszString[nLeftPos] == '"' )
	{
		const int nRightPos = rszString.find_first_of( '"', nLeftPos + 1 );
		szString = rszString.substr( nLeftPos + 1, nRightPos - nLeftPos - 1 );
	}
	else
	{
		const int nRightPos = rszString.find_first_of( rszDividers, nLeftPos );
		szString = rszString.substr( nLeftPos, nRightPos - nLeftPos );
	}
	//
	int nValue = nDefaultValue;
	if ( sscanf( szString.c_str(), "%d", &nValue ) == 1 )
	{
		return nValue;
	}
	else
	{
		return nDefaultValue;
	}
}


float CStringManager::GetFloatValueFromString( const std::string &rszString, const std::string &rszLabel, const int nPos, const std::string &rszDividers, float fDefaultValue )
{
	const int nStringPos = rszString.find( rszLabel, nPos );
	if ( nStringPos == std::string::npos )
	{
		return fDefaultValue;
	}
	const int nLeftPos = rszString.find_first_not_of( rszDividers, nStringPos + rszLabel.size() );
	if ( nLeftPos == std::string::npos )
	{
		return fDefaultValue;
	}
	std::string szString;
	if ( rszString[nLeftPos] == '"' )
	{
		const int nRightPos = rszString.find_first_of( '"', nLeftPos + 1 );
		szString = rszString.substr( nLeftPos + 1, nRightPos - nLeftPos - 1 );
	}
	else
	{
		const int nRightPos = rszString.find_first_of( rszDividers, nLeftPos );
		szString = rszString.substr( nLeftPos, nRightPos - nLeftPos );
	}
	//
	float fValue = fDefaultValue;
	if ( sscanf( szString.c_str(), "%g", &fValue ) == 1 )
	{
		return fValue;
	}
	else
	{
		return fDefaultValue;
	}
}


bool CStringManager::GetBoolValueFromString( const std::string &rszString, const std::string &rszLabel, const int nPos, const std::string &rszDividers, bool bDefaultValue )
{
	const int nStringPos = rszString.find( rszLabel, nPos );
	if ( nStringPos == std::string::npos )
	{
		return bDefaultValue;
	}
	const int nLeftPos = rszString.find_first_not_of( rszDividers, nStringPos + rszLabel.size() );
	if ( nLeftPos == std::string::npos )
	{
		return bDefaultValue;
	}
	std::string szString;
	if ( rszString[nLeftPos] == '"' )
	{
		const int nRightPos = rszString.find_first_of( '"', nLeftPos + 1 );
		szString = rszString.substr( nLeftPos + 1, nRightPos - nLeftPos - 1 );
	}
	else
	{
		const int nRightPos = rszString.find_first_of( rszDividers, nLeftPos );
		szString = rszString.substr( nLeftPos, nRightPos - nLeftPos );
	}
	//
	return ( szString == "true" );
}


int CStringManager::NormalizeValue( int nValue, int nStep )
{
	if ( ( nValue % nStep ) != 0 )
	{
		const int nLower = nValue % nStep;
		const int nUpper = nStep - ( nValue % nStep );
		if ( nLower < nUpper )
		{
			nValue -= nLower;
		}
		else
		{
			nValue += nUpper;
		}
	}
	return nValue;
}


int CStringManager::GetPowerPrecision( int nPrercision )
{
	int nPowerPrecision = 1;
	for ( int nPrecisionIndex = 0; nPrecisionIndex < nPrercision; ++nPrecisionIndex )
	{
		nPowerPrecision *= 10;	
	}
	return nPowerPrecision;
}


void CStringManager::GetTypeAndNameFromRefValue( std::string *pszTypeName, std::string *pszName, const std::string &rszRefValue, char cSeparator, const std::string &rszDefaultTypeName )
{
	//NI_ASSERT( pszTypeName != 0, "CStringManager::GetTypeAndNameFromRefValue(): pszTypeName == 0" );
	//NI_ASSERT( pszName != 0, "CStringManager::GetTypeAndNameFromRefValue(): pszName == 0" );
	//
	const int nPos = rszRefValue.find( cSeparator );
	if ( nPos == std::string::npos )
	{
		if ( pszTypeName )
		{
			( *pszTypeName ) = NDb::GetClassTypeName( CDBID( rszRefValue ) );
		}
		if ( pszName )
		{
			( *pszName ) = rszRefValue;
		}
	}
	else
	{
		if ( pszTypeName )
		{
			( *pszTypeName ) = rszRefValue.substr( 0, nPos );
		}
		if ( pszName )
		{
			( *pszName ) = rszRefValue.substr( nPos + 1 );
		}
	}
}


void CStringManager::GetRefValueFromTypeAndName( std::string *pszRefValue, const std::string &rszTypeName, const std::string &rszName, char cSeparator )
{
	NI_ASSERT( pszRefValue != 0, "CStringManager::GetRefValueFromTypeAndName(): pszRefValue == 0" );
	//
	( *pszRefValue ) = rszTypeName + cSeparator + rszName;
}


void CStringManager::CutFileName( std::string *pszFileName )
{
	NI_ASSERT( pszFileName != 0, "CStringManager::CutFileName(): pszFileName == 0" );
	//
	NStr::ReplaceAllChars( pszFileName, '/', '\\' );
	const int nPos = pszFileName->rfind( '\\' );
	if ( nPos >= 0 )
	{
		( *pszFileName ) = pszFileName->substr( 0, nPos );
		( *pszFileName ) += std::string( "\\" );
	}
	else
	{
		pszFileName->clear();
	}
}


bool CStringManager::CutFileExtention( std::string *pszFileName )
{
	if ( pszFileName )
	{
		const int nDotPosition = pszFileName->rfind( '.' );
		if ( nDotPosition > 0 )
		{
			( *pszFileName ) = pszFileName->substr( 0, nDotPosition );
			return true;
		}
		else if ( nDotPosition == 0 )
		{
			pszFileName->clear();
			return true;
		}
	}
	return false;
}


bool CStringManager::CutFileExtention( std::string *pszFileName, const std::string &rszFileExtention )
{
	if ( pszFileName )
	{
		std::string szFileNameNoCase = ( *pszFileName );
		std::string szFileExtentionNoCase = rszFileExtention;
		NStr::ToLower( &szFileNameNoCase );
		NStr::ToLower( &szFileExtentionNoCase );
		const int nDotPosition = szFileNameNoCase.rfind( '.' );
		const std::string szCurrentFileExtentionNoCase = szFileNameNoCase.substr( ( nDotPosition != std::string::npos ) ? nDotPosition : 0, std::string::npos );
		if ( szCurrentFileExtentionNoCase == szFileExtentionNoCase )
		{
			if ( nDotPosition > 0 )
			{
				( *pszFileName ) = pszFileName->substr( 0, nDotPosition );
				return true;
			}
			else if ( nDotPosition == 0 )
			{
				pszFileName->clear();
				return true;
			}
		}
	}
	return false;
}


void CStringManager::ExtendFileExtention( std::string *pszFileName, const std::string &rszFileExtention )
{
	if ( pszFileName )
	{
		std::string szFileNameNoCase = ( *pszFileName );
		std::string szFileExtentionNoCase = rszFileExtention;
		NStr::ToLower( &szFileNameNoCase );
		NStr::ToLower( &szFileExtentionNoCase );
		const int nDotPosition = szFileNameNoCase.rfind( '.' );
		const std::string szCurrentFileExtentionNoCase = szFileNameNoCase.substr( ( nDotPosition != std::string::npos ) ? nDotPosition : 0, std::string::npos );
		if ( szCurrentFileExtentionNoCase != szFileExtentionNoCase )
		{
			( *pszFileName ) += rszFileExtention;
		}
	}
}


void CStringManager::ExtendFileExtention( CString *pstrFileName, const CString &rstrFileExtention )
{
	if ( pstrFileName )
	{
		std::string szFileName = ( *pstrFileName );
		std::string szFileExtention = rstrFileExtention;
		ExtendFileExtention( &szFileName, szFileExtention );
		( *pstrFileName ) = szFileName.c_str();
	}
}


void CStringManager::SplitFileName( std::string *pszFilePath, std::string *pszFileName, std::string *pszFileExtention, const std::string &rszFullFileName )
{
	std::string szFullFileName = rszFullFileName;
	NStr::ReplaceAllChars( &szFullFileName, '/', '\\' );
	std::string szFilePath;
	std::string szFileName;
	std::string szFileExtention;
	const int nDotPosition = szFullFileName.rfind( '.' );
	if ( nDotPosition != std::string::npos )
	{
		szFileExtention = szFullFileName.substr( nDotPosition, std::string::npos );
		szFileName = szFullFileName.substr( 0, nDotPosition );
	}
	else
	{
		szFileName = szFullFileName;
	}
	const int nSlashPosition = szFileName.rfind( '\\' );
	if ( nSlashPosition != std::string::npos )
	{
		szFilePath = szFileName.substr( 0, nSlashPosition + 1 );
		szFileName = szFileName.substr( nSlashPosition + 1, std::string::npos );
	}
	//
	if ( pszFilePath )
	{
		 ( *pszFilePath	 ) = szFilePath;
	}
	if ( pszFileName )
	{
		 ( *pszFileName	 ) = szFileName;
	}
	if ( pszFileExtention )
	{
		 ( *pszFileExtention	 ) = szFileExtention;
	}
}


void CStringManager::RemoveDoubleSlashes( std::string *pszFilePath )
{
  if ( pszFilePath && !pszFilePath->empty() )
	{
		NStr::ReplaceAllChars( pszFilePath, '/', '\\' );
		std::string::iterator itChar = pszFilePath->begin();
		std::string::iterator itNextChar = pszFilePath->begin();
		++itNextChar;
		for ( ; itNextChar != pszFilePath->end(); )
		{
			if ( ( ( *itChar ) == '\\' ) && ( ( *itNextChar ) == '\\' ) )
			{
				itChar = pszFilePath->erase( itChar );
				itNextChar = itChar;
				++itNextChar;
			}
			else
			{
				++itChar;
				++itNextChar;
			}
		}
	}
}


std::string CStringManager::GetFloatStringWithPrecision( const float fValue, const int nPrecision )
{
	if ( nPrecision > 0 ) 
	{
		const std::string szFormat = fmt::format( "%.{}f", nPrecision );
		return StrFmt( szFormat.c_str(), fValue );
	}
	else
	{
		return fmt::format( "{:g}", fValue );
	}
}


int CStringManager::Compare( const std::string &rszLeft, const std::string &rszRight, bool bIgnoreCase, bool bIgnoreSlash, bool bSubString )
{
	std::string szLeft = rszLeft;
	std::string szRight = rszRight;
	if ( bIgnoreCase )
	{
		NStr::ToLower( &szLeft );
		NStr::ToLower( &szRight );
	}
	if ( bIgnoreSlash )
	{
		NStr::ReplaceAllChars( &szLeft, '/', '\\' );
		NStr::ReplaceAllChars( &szRight, '/', '\\' );
	}
	if ( bSubString )
	{
		return szLeft.compare( 0, szRight.size(), szRight );
	}
	else
	{
		return szLeft.compare( szRight );
	}
}

// basement storage  


