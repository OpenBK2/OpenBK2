#include "stdafx.h"
#include "ResourceDefines.h"

#include "UserDataContainer.h"
#include "Main/MainLoop.h"
#include "System/FilePath.h"
#include "Misc/StrProc.h"

#define START_FOLDER_TOKEN "%START%\\"

typedef std::unordered_map<std::string, std::string> CTokensMap;
static CTokensMap s_TokensMap;

void AddToken( const std::string &szName, const std::string &szValue )
{
	s_TokensMap[szName] = szValue;
}

void ReplaceToken( std::string *pInput, const std::string &szToken, const std::string &szReplace )
{
	const int nPos = pInput->find( szToken );
	if ( nPos != std::string::npos )
		pInput->replace( pInput->begin() + nPos, pInput->begin() + szToken.size(), szReplace );
}

void ReplaceTokens( std::string *pszString )
{
	const int nPos = pszString->find( '%' );
	if ( nPos == std::string::npos )
		return;
	const int nLastPos = pszString->find( '%', nPos + 1 );
	const std::string szTokenName = pszString->substr( nPos + 1, nLastPos - nPos - 1 );
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


void CUserDataContainer::GetXMLFilePath( std::string *pszXMLFilePath )
{
	NI_ASSERT( pszXMLFilePath != 0, "CUserDataContainer::GetXMLFilePath() pszXMLFilePath is NULL" );
	( *pszXMLFilePath ) = NFile::JoinPath( "Editor", "UserData" );
}


void CUserDataContainer::GetConstXMLFilePath( std::string *pszConstXMLFilePath )
{
	NI_ASSERT( pszConstXMLFilePath != 0, "CUserDataContainer::GetXMLFilePath() pszXMLFilePath is NULL" );
	( *pszConstXMLFilePath ) = NFile::JoinPath( "Editor", "ConstUserData" );
}


void CUserDataContainer::Load()
{
	std::string szStartFolder = NMainLoop::GetBaseDir();
	// AppendSlash rather than the test this used to make, which asked only
	// whether the last character was a backslash. GetBaseDir ends in a forward
	// one, so off Windows the answer was no and a backslash went on the end of
	// an otherwise good path: "/home/sse4/bk2/\".
	NFile::AppendSlash( &szStartFolder, NFile::PATH_SEPARATOR );
	std::string szTokenSTART = szStartFolder.substr( 0, szStartFolder.size() - 1 );
	NFile::NormalizePath( &szTokenSTART );
	AddToken( "BasePath", szTokenSTART );
	//
	std::string szXMLFilePath;
	GetXMLFilePath( &szXMLFilePath );
	LoadXMLResource( szStartFolder + szXMLFilePath, ".xml", "UserData", userData );
	// загружаем константную часть
	std::string szConstXMLFilePath;
	GetConstXMLFilePath( &szConstXMLFilePath );
	LoadXMLResource(  szStartFolder + szConstXMLFilePath, ".xml", "ConstUserData", userData.constUserData );
	{
		// Стартовый каталог
		userData.constUserData.szStartFolder = szStartFolder;
		// check for tokens in folder pathes
		//
		// These used to be folded the other way, to backslashes, which is how
		// DataStorageFolder came out as "\home\sse4\bk2\\Data\" -- an absolute
		// path turned into one nonexistent filename. Folding to
		// PATH_SEPARATOR keeps the normalising, which the XML still needs since
		// a file written by a Windows editor has backslashes in it, and points
		// it in the direction FilePath.h says paths go in this tree.
		ReplaceTokens( &userData.constUserData.szObjectRecordIDsFolder );
		NStr::ReplaceAllChars( &userData.constUserData.szObjectRecordIDsFolder, '\\', NFile::PATH_SEPARATOR );
		ReplaceTokens( &userData.constUserData.szExportSourceFolder );
		NStr::ReplaceAllChars( &userData.constUserData.szExportSourceFolder, '\\', NFile::PATH_SEPARATOR );
		ReplaceTokens( &userData.constUserData.szExportDestinationFolder );
		NStr::ReplaceAllChars( &userData.constUserData.szExportDestinationFolder, '\\', NFile::PATH_SEPARATOR );
		// DataStorageFolder must be reconstructed from szStartFolder and "Data" dir
		userData.constUserData.szDataStorageFolder = NFile::JoinPath( szStartFolder, NFile::DIR_DATA );
		NFile::AppendSlash( &userData.constUserData.szDataStorageFolder, NFile::PATH_SEPARATOR );
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
	std::string szXMLFilePath;
	GetXMLFilePath( &szXMLFilePath );
	SaveXMLResource( userData.constUserData.szStartFolder + szXMLFilePath, ".xml", "UserData", userData );
}


// basement storage  


