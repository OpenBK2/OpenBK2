#include "stdafx.h"
#include <fmt/format.h>
#include "ResourceDefines.h"

#include "MapEditorLib/Tools_Registry.h"
#include "MapEditorLib/Interface_UserData.h"

#include "MainFrameParams.h"

SMainFrameParams::SMainFrameParams() 
	: bMaximized( false )
{
}


void SMainFrameParams::GetRegistryKey( std::string *pszRegistryKey )
{
	NI_ASSERT( pszRegistryKey != 0, "SMainFrameParams::GetRegistryKey() pszRegistryKey is NULL" );
	CString strPath;
	strPath.LoadString( IDS_REGISTRY_PATH );
	CString strTitle;
	strTitle.LoadString( AFX_IDS_APP_TITLE );
	CString strKey;
	strKey.LoadString( IDS_REGISTRY_KEY );
	( *pszRegistryKey ) = fmt::format( "Software\\{}\\{}\\{}\\{}",
																LPCTSTR( strPath ),
																Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(),
																LPCTSTR( strTitle ),
																LPCTSTR( strKey ) );
}


void SMainFrameParams::GetXMLFilePath( std::string *pszXMLFilePath )
{
	NI_ASSERT( pszXMLFilePath != 0, "SMainFrameParams::GetXMLFilePath() pszXMLFilePath is NULL" );
	( *pszXMLFilePath ) = "Editor\\MainFrameParams";
}


int SMainFrameParams::operator&( IBinSaver &bs )
{
	bs.Add( 1, &bMaximized );
	bs.Add( 2, &rect );
	return 0;
}


int SMainFrameParams::operator&( IXmlSaver &xs )
{
	xs.Add( "FullScreen", &bMaximized );
	xs.Add( "Rect", &rect );
	return 0;
}


void SMainFrameParams::Load( bool bFromRegistry )
{
	if ( bFromRegistry )
	{
		std::string szRegistryKey;
		GetRegistryKey( &szRegistryKey );
		CRegistrySection registrySection( HKEY_CURRENT_USER, KEY_READ, szRegistryKey.c_str() );

		CString strKey;
		std::string szFormat;
		int nValue = 0;
		std::string szValue;

		nValue = 0;
		strKey.LoadString( IDS_REGISTRY_KEY_MAXIMIZE );
		registrySection.LoadNumber( strKey, "%d", &nValue, 0 );
		bMaximized = ( nValue  > 0 );

		strKey.LoadString( IDS_REGISTRY_KEY_RECT );
		registrySection.LoadRect( strKey, "%d", &rect, CTRect<int>( 0, 0, 0, 0 ) );
		
		/**
		// recentList
		{
			int nRecentCount = 0;
			strKey.LoadString( IDS_REGISTRY_KEY_RECENT_LIST );
			szFormat = fmt::format( "{}s", LPCTSTR( strKey ) );
			registrySection.LoadNumber( szFormat.c_str(), "%d", &nRecentCount, 0 );
			recentList.clear();
			for ( int nRecentIndex = 0; nRecentIndex < nRecentCount; ++nRecentIndex )
			{
				std::string szFormat = fmt::format( "{}{}", LPCTSTR( strKey ), nRecentIndex );
				szValue.clear();
				registrySection.LoadString( szFormat.c_str(), &szValue, "" );
				recentList.push_back( szValue );
			}
		}
		// tables
		{
			int nTablesCount = 0;
			strKey.LoadString( IDS_REGISTRY_KEY_TABLE );
			szFormat = fmt::format( "{}s", LPCTSTR( strKey ) );
			registrySection.LoadNumber( szFormat.c_str(), "%d", &nTablesCount, 0 );
			tables.clear();
			for ( int nTableIndex = 0; nTableIndex < nTablesCount; ++nTableIndex )
			{
				std::string szFormat = fmt::format( "{}{}", LPCTSTR( strKey ), nTableIndex );
				szValue.clear();
				registrySection.LoadString( szFormat.c_str(), &szValue, "" );
				InsertHashSetElement( &tables, szValue );
			}
		}
		strKey.LoadString( IDS_REGISTRY_CURRENT_TABLE );
		registrySection.LoadString( strKey, &szCurrentTable, "" );
		/**/
	}
	else
	{
		std::string szXMLFilePath;
		GetXMLFilePath( &szXMLFilePath );
		LoadXMLResource( Singleton<IUserDataContainer>()->Get()->constUserData.szStartFolder + szXMLFilePath, ".xml", "MainFrameParams", ( *this ) );
	}
}


void SMainFrameParams::Save(  bool bToRegistry )
{
	if ( bToRegistry )
	{
		std::string szRegistryKey;
		GetRegistryKey( &szRegistryKey );
		::RegDeleteKey( HKEY_CURRENT_USER, szRegistryKey.c_str() );
		CRegistrySection registrySection( HKEY_CURRENT_USER, KEY_WRITE, szRegistryKey.c_str() );

		CString strKey;
		std::string szFormat;

		strKey.LoadString( IDS_REGISTRY_KEY_MAXIMIZE );
		registrySection.SaveNumber( strKey, "%d", bMaximized );

		strKey.LoadString( IDS_REGISTRY_KEY_RECT );
		registrySection.SaveRect( strKey, "%d", rect );

		/**
		// recentList
		{
			int nRecentCount = recentList.size();
			strKey.LoadString( IDS_REGISTRY_KEY_RECENT_LIST );
			szFormat = fmt::format( "{}s", LPCTSTR( strKey ) );
			registrySection.SaveNumber( szFormat.c_str(), "%d", nRecentCount );
			int nRecentIndex = 0;
			for ( std::list<std::string>::const_iterator itRecent = recentList.begin(); itRecent != recentList.end(); ++itRecent )
			{
				std::string szFormat = fmt::format( "{}{}", LPCTSTR( strKey ), nRecentIndex );
				registrySection.SaveString( szFormat.c_str(), ( *itRecent ) );
				++nRecentIndex;
			}
		}

		// tables
		{
			int nTablesCount = tables.size();
			strKey.LoadString( IDS_REGISTRY_KEY_TABLE );
			szFormat = fmt::format( "{}s", LPCTSTR( strKey ) );
			registrySection.SaveNumber( szFormat.c_str(), "%d", nTablesCount );
			int nTableIndex = 0;
			for ( CTableSet::const_iterator itTable = tables.begin(); itTable != tables.end(); ++itTable )
			{
				std::string szFormat = fmt::format( "{}{}", LPCTSTR( strKey ), nTableIndex );
				registrySection.SaveString( szFormat.c_str(), itTable->first );
				++nTableIndex;
			}
		}
		strKey.LoadString( IDS_REGISTRY_CURRENT_TABLE );
		registrySection.SaveString( strKey, szCurrentTable );
		/**/
	}
	else
	{
		std::string szXMLFilePath;
		GetXMLFilePath( &szXMLFilePath );
		SaveXMLResource( Singleton<IUserDataContainer>()->Get()->constUserData.szStartFolder + szXMLFilePath, ".xml", "MainFrameParams", ( *this ) );
	}
}


// basement storage  


