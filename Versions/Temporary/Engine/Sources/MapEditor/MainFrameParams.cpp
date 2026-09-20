#include "stdafx.h"
#include "MapEditorLib/Resources.h"
#include <fmt/format.h>
#include "ResourceDefines.h"

#include <fmt/printf.h>

#include <cstdio>
#include "MapEditorLib/Interface_UserData.h"

#include "MainFrameParams.h"
#include "AppProfile.h"

SMainFrameParams::SMainFrameParams() 
	: bMaximized( false )
{
}


// The section of the application's profile these live in.
std::string SMainFrameParams::GetSection()
{
	return NResources::GetString( IDS_REGISTRY_KEY );
}


namespace
{
	// The text CRegistrySection wrote, kept exactly: it stored numbers and
	// rectangles as strings, so what an earlier build saved still reads back.
	int ReadNumber( const std::string &rszSection, const std::string &rszEntry, int nDefault )
	{
		const std::string szText = NAppProfile::GetString( rszSection, rszEntry, "" );
		int nValue = nDefault;
		if ( szText.empty() || ( sscanf( szText.c_str(), "%d", &nValue ) < 1 ) )
		{
			return nDefault;
		}
		return nValue;
	}


	CTRect<int> ReadRect( const std::string &rszSection, const std::string &rszEntry, const CTRect<int> &rDefault )
	{
		const std::string szText = NAppProfile::GetString( rszSection, rszEntry, "" );
		CTRect<int> rect = rDefault;
		if ( szText.empty() ||
				 ( sscanf( szText.c_str(), "%d %d %d %d", &rect.minx, &rect.miny, &rect.maxx, &rect.maxy ) < 4 ) )
		{
			return rDefault;
		}
		return rect;
	}
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
		const std::string szSection = GetSection();
		std::string strKey;
		std::string szFormat;
		std::string szValue;

		strKey = NResources::GetString( IDS_REGISTRY_KEY_MAXIMIZE );
		bMaximized = ( ReadNumber( szSection, strKey, 0 ) > 0 );

		strKey = NResources::GetString( IDS_REGISTRY_KEY_RECT );
		rect = ReadRect( szSection, strKey, CTRect<int>( 0, 0, 0, 0 ) );
		
		/**
		// recentList
		{
			int nRecentCount = 0;
			strKey = NResources::GetString( IDS_REGISTRY_KEY_RECENT_LIST );
			szFormat = fmt::format( "{}s", strKey.c_str() );
			registrySection.LoadNumber( szFormat.c_str(), "%d", &nRecentCount, 0 );
			recentList.clear();
			for ( int nRecentIndex = 0; nRecentIndex < nRecentCount; ++nRecentIndex )
			{
				std::string szFormat = fmt::format( "{}{}", strKey.c_str(), nRecentIndex );
				szValue.clear();
				registrySection.LoadString( szFormat.c_str(), &szValue, "" );
				recentList.push_back( szValue );
			}
		}
		// tables
		{
			int nTablesCount = 0;
			strKey = NResources::GetString( IDS_REGISTRY_KEY_TABLE );
			szFormat = fmt::format( "{}s", strKey.c_str() );
			registrySection.LoadNumber( szFormat.c_str(), "%d", &nTablesCount, 0 );
			tables.clear();
			for ( int nTableIndex = 0; nTableIndex < nTablesCount; ++nTableIndex )
			{
				std::string szFormat = fmt::format( "{}{}", strKey.c_str(), nTableIndex );
				szValue.clear();
				registrySection.LoadString( szFormat.c_str(), &szValue, "" );
				InsertHashSetElement( &tables, szValue );
			}
		}
		strKey = NResources::GetString( IDS_REGISTRY_CURRENT_TABLE );
		registrySection.LoadString( strKey.c_str(), &szCurrentTable, "" );
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
		const std::string szSection = GetSection();
		std::string strKey;
		std::string szFormat;

		strKey = NResources::GetString( IDS_REGISTRY_KEY_MAXIMIZE );
		NAppProfile::WriteString( szSection, strKey, fmt::sprintf( "%d", bMaximized ? 1 : 0 ) );

		strKey = NResources::GetString( IDS_REGISTRY_KEY_RECT );
		NAppProfile::WriteString( szSection, strKey,
															fmt::sprintf( "%d %d %d %d", rect.minx, rect.miny, rect.maxx, rect.maxy ) );

		/**
		// recentList
		{
			int nRecentCount = recentList.size();
			strKey = NResources::GetString( IDS_REGISTRY_KEY_RECENT_LIST );
			szFormat = fmt::format( "{}s", strKey.c_str() );
			registrySection.SaveNumber( szFormat.c_str(), "%d", nRecentCount );
			int nRecentIndex = 0;
			for ( std::list<std::string>::const_iterator itRecent = recentList.begin(); itRecent != recentList.end(); ++itRecent )
			{
				std::string szFormat = fmt::format( "{}{}", strKey.c_str(), nRecentIndex );
				registrySection.SaveString( szFormat.c_str(), ( *itRecent ) );
				++nRecentIndex;
			}
		}

		// tables
		{
			int nTablesCount = tables.size();
			strKey = NResources::GetString( IDS_REGISTRY_KEY_TABLE );
			szFormat = fmt::format( "{}s", strKey.c_str() );
			registrySection.SaveNumber( szFormat.c_str(), "%d", nTablesCount );
			int nTableIndex = 0;
			for ( CTableSet::const_iterator itTable = tables.begin(); itTable != tables.end(); ++itTable )
			{
				std::string szFormat = fmt::format( "{}{}", strKey.c_str(), nTableIndex );
				registrySection.SaveString( szFormat.c_str(), itTable->first );
				++nTableIndex;
			}
		}
		strKey = NResources::GetString( IDS_REGISTRY_CURRENT_TABLE );
		registrySection.SaveString( strKey.c_str(), szCurrentTable );
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


