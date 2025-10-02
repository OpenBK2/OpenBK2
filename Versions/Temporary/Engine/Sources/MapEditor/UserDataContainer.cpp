#include "StdAfx.h"
#include "ResourceDefines.h"

#include "UserDataContainer.h"
#include "Main/MainLoop.h"
#include "System/FilePath.h"
#include "Misc/StrProc.h"

#define START_FOLDER_TOKEN "%START%\\"

typedef hash_map<string, string> CTokensMap;
static CTokensMap s_TokensMap;

void AddToken( const string &szName, const string &szValue )
{
	s_TokensMap[szName] = szValue;
}

void ReplaceToken( string *pInput, const string &szToken, const string &szReplace )
{
	const int nPos = pInput->find( szToken );
	if ( nPos != string::npos )
		pInput->replace( pInput->begin() + nPos, pInput->begin() + szToken.size(), szReplace );
}

void ReplaceTokens( string *pszString )
{
	const int nPos = pszString->find( '%' );
	if ( nPos == string::npos )
		return;
	const int nLastPos = pszString->find( '%', nPos + 1 );
	const string szTokenName = pszString->substr( nPos + 1, nLastPos - nPos - 1 );
	CTokensMap::const_iterator posToken = s_TokensMap.find( szTokenName );
	if ( posToken != s_TokensMap.end() )
		pszString->replace( pszString->begin() + nPos, pszString->begin() + nLastPos + 1, posToken->second );		
	ReplaceTokens( pszString );
}

CUserDataContainer::CUserDataContainer()
{
}


CUserDataContainer::~CUserDataContainer()
{
}


void CUserDataContainer::GetXMLFilePath( string *pszXMLFilePath )
{
	NI_ASSERT( pszXMLFilePath != 0, "CUserDataContainer::GetXMLFilePath() pszXMLFilePath is NULL" );
	( *pszXMLFilePath ) = "Editor\\UserData";
}


void CUserDataContainer::GetConstXMLFilePath( string *pszConstXMLFilePath )
{
	NI_ASSERT( pszConstXMLFilePath != 0, "CUserDataContainer::GetXMLFilePath() pszXMLFilePath is NULL" );
	( *pszConstXMLFilePath ) = "Editor\\ConstUserData";
}


void CUserDataContainer::Load()
{
	string szStartFolder = NMainLoop::GetBaseDir();
	if ( ( !szStartFolder.empty() ) &&
			 ( szStartFolder[ szStartFolder.size() - 1] != '\\' ) )
	{
		szStartFolder += '\\';
	}
	string szTokenSTART = szStartFolder.substr( 0, szStartFolder.size() - 1 );
	NFile::NormalizePath( &szTokenSTART );
	AddToken( "BasePath", szTokenSTART );
	//
	string szXMLFilePath;
	GetXMLFilePath( &szXMLFilePath );
	LoadXMLResource( szStartFolder + szXMLFilePath, ".xml", "UserData", userData );
	// загружаем константную часть
	string szConstXMLFilePath;
	GetConstXMLFilePath( &szConstXMLFilePath );
	LoadXMLResource(  szStartFolder + szConstXMLFilePath, ".xml", "ConstUserData", userData.constUserData );
	{
		// Стартовый каталог
		userData.constUserData.szStartFolder = szStartFolder;
		// check for tokens in folder pathes
		ReplaceTokens( &userData.constUserData.szObjectRecordIDsFolder );
		NStr::ReplaceAllChars( &userData.constUserData.szObjectRecordIDsFolder, '/', '\\' );
		ReplaceTokens( &userData.constUserData.szExportSourceFolder );
		NStr::ReplaceAllChars( &userData.constUserData.szExportSourceFolder, '/', '\\' );
		ReplaceTokens( &userData.constUserData.szExportDestinationFolder );
		NStr::ReplaceAllChars( &userData.constUserData.szExportDestinationFolder, '/', '\\' );
		// DataStorageFolder must be reconstructed from szStartFolder and "Data" dir
		userData.constUserData.szDataStorageFolder = szStartFolder + "Data/";
		NStr::ReplaceAllChars( &userData.constUserData.szDataStorageFolder, '/', '\\' );
		//
		ReplaceTokens( &userData.constUserData.mayaExportData.szMayaExportPath );
		AddToken( "MayaExportPath", userData.constUserData.mayaExportData.szMayaExportPath );
		ReplaceTokens( &userData.constUserData.mayaExportData.szMayaVersionPath );
		AddToken( "MayaVersionPath", userData.constUserData.mayaExportData.szMayaVersionPath );

		ReplaceTokens( &userData.constUserData.mayaExportData.szMayaScriptPath );
		ReplaceTokens( &userData.constUserData.mayaExportData.szStartupScript );
		ReplaceTokens( &userData.constUserData.mayaExportData.szOldPluginFileName );
		ReplaceTokens( &userData.constUserData.mayaExportData.szOldPluginName );
		for ( SUserData::SMayaExportData::CGrannyExportSettingsMap::iterator it = userData.constUserData.mayaExportData.grannyExportSettings.begin();
			it != userData.constUserData.mayaExportData.grannyExportSettings.end(); ++it )
		{
			ReplaceTokens( &it->second );
		}
	}
}


void CUserDataContainer::Save()
{
	// replace start path with token
	ReplaceToken( &userData.constUserData.szExportSourceFolder, userData.constUserData.szStartFolder, START_FOLDER_TOKEN );
	ReplaceToken( &userData.constUserData.szExportDestinationFolder, userData.constUserData.szStartFolder, START_FOLDER_TOKEN );
	ReplaceToken( &userData.constUserData.szDataStorageFolder, userData.constUserData.szStartFolder, START_FOLDER_TOKEN );
	//
	string szXMLFilePath;
	GetXMLFilePath( &szXMLFilePath );
	SaveXMLResource( userData.constUserData.szStartFolder + szXMLFilePath, ".xml", "UserData", userData );
}


// basement storage  


