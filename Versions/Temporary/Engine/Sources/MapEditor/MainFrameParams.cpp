#include "stdafx.h"

#include "MainFrameParams.h"

#include "AppProfile.h"
#include "ResourceDefines.h"
#include "MapEditorLib/Resources.h"

#include <fmt/printf.h>

#include <cstdio>

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


void SMainFrameParams::Load()
{
	const std::string szSection = GetSection();
	std::string strKey;

	strKey = NResources::GetString( IDS_REGISTRY_KEY_MAXIMIZE );
	bMaximized = ( ReadNumber( szSection, strKey, 0 ) > 0 );

	strKey = NResources::GetString( IDS_REGISTRY_KEY_RECT );
	rect = ReadRect( szSection, strKey, CTRect<int>( 0, 0, 0, 0 ) );
}


void SMainFrameParams::Save()
{
	const std::string szSection = GetSection();
	std::string strKey;

	strKey = NResources::GetString( IDS_REGISTRY_KEY_MAXIMIZE );
	NAppProfile::WriteString( szSection, strKey, fmt::sprintf( "%d", bMaximized ? 1 : 0 ) );

	strKey = NResources::GetString( IDS_REGISTRY_KEY_RECT );
	NAppProfile::WriteString( szSection, strKey,
														fmt::sprintf( "%d %d %d %d", rect.minx, rect.miny, rect.maxx, rect.maxy ) );
}


// basement storage  


