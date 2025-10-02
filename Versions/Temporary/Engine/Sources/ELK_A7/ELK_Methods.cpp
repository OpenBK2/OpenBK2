#include "StdAfx.h"
#include "resource.h"

#include "ELK_Types.h"
#include "../MapEditorLib/Tools_Resources.h"
#include "../MapEditorLib/StringManager.h"
#include "../Misc/StrProc.h"
#include "../System/WinVFS.h"
#include "../System/VFSOperations.h"
#include "CreateFilterDialog.h"
#include "../System/XmlSaver.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


int SELKTextProperty::operator&( IXmlSaver &xs )
{
	xs.Add( "State", &nState );
	xs.Add( "Changed", &bTranslated );
	return 0;
}


int SELKDescription::operator&( IXmlSaver &xs )
{
	xs.Add( "Name", &szName );
	xs.Add( "PAK", &szPAKName );
	xs.Add( "UPD", &szUPDPrefix );
	xs.Add( "Fonts", &bGenerateFonts );
	xs.Add( "Chars", &bUsedChars );
	return 0;
}


int SELKElement::operator&( IXmlSaver &xs )
{
	xs.Add( "Description", &description );
	xs.Add( "Path", &szPath );
	xs.Add( "Version", &szVersion );
	xs.Add( "LastUpdateNumber", &nLastUpdateNumber );
	return 0;
}


int SELKElementStatistic::SState::operator&( IXmlSaver &xs )
{
	xs.Add( "TextsCount", &nTextsCount );
	xs.Add( "WordCount", &nWordsCount );
	xs.Add( "WordSymbolsCount", &nWordSymbolsCount );
	xs.Add( "SymbolsCount", &nSymbolsCount );
	return 0;
}


int SELKElementStatistic::operator&( IXmlSaver &xs )
{
	xs.Add( "States", &states );
	return 0;
}


int SELKStatistic::operator&( IXmlSaver &xs )
{
	xs.Add( "Orininal", &original );
	xs.Add( "Translation", &translation );
	xs.Add( "Valid", &bValid );
	return 0;
}


int CELK::operator&( IXmlSaver &xs )
{
	xs.Add( "Elements", &elementList );
	xs.Add( "Path", &szPath );
	xs.Add( "PreviousStatistic", &previousStatistic );
	xs.Add( "Statistic", &statistic );
	return 0;
}


void SELKElement::GetDataBaseFolder( string *pszDataBaseFolder ) const
{
	GetDataBaseFolder( szPath, pszDataBaseFolder );
}


void SELKElement::GetDataBaseReserveFolder( string *pszDataBaseReserveFolder ) const
{
	GetDataBaseReserveFolder( szPath, pszDataBaseReserveFolder );
}


bool CELK::Open( const string &rszELKPath, bool bEnumFiles )
{
	Close();
	LoadTypedSuperXMLResource( rszELKPath, XML_EXTENTION, ( *this ) );
	//
	szPath = rszELKPath;
	
	string szCurrentFolder = szPath.substr( 0, szPath.rfind( '\\' ) + 1 );
	for ( int nElementIndex = 0; nElementIndex < elementList.size(); ++nElementIndex )
	{
		string szElementName = elementList[nElementIndex].szPath.substr( elementList[nElementIndex].szPath.rfind( '\\' ) + 1 );
		elementList[nElementIndex].szPath = szCurrentFolder + szElementName;
		elementNameMap[szElementName] = nElementIndex;
	}

	if ( bEnumFiles )
	{
		for ( int nElementIndex = 0; nElementIndex < elementList.size(); ++nElementIndex )
		{
			string szDataBaseFolder;
			elementList[nElementIndex].GetDataBaseFolder( &szDataBaseFolder );

			CObj<NVFS::IVFS> pVFS = NVFS::CreateWinVFS( szDataBaseFolder + "*.pak" );
			NVFS::SetMainVFS( pVFS );
			EnumFilesInDataStorage( 0, &enumFolderStructureParameter );
		}
	}
	return IsOpened();
}


bool CELK::Save()
{
	if ( IsOpened() )
	{
		SaveTypedSuperXMLResource( szPath, XML_EXTENTION, ( *this ) );
	}
	return true;
}


void CELK::Close()
{
	if ( IsOpened() )
	{
		if ( statistic.bValid )
		{
			previousStatistic = statistic;
		}
		Save();
		previousStatistic.Clear();
		statistic.Clear();
		szPath.clear();
		elementList.clear();
		elementNameMap.clear();

		enumFolderStructureParameter.nIgnoreFolderCount = 1;
		enumFolderStructureParameter.enumFolderMap.clear();
	}
}


bool SSimpleFilter::Check( const string &rszFolder, bool _bTranslated, int nState ) const
{
	bool bChecked = false;
	if ( !filter.empty() )
	{
		string szFolder = rszFolder;
		NStr::ToLower( &szFolder );
		for ( CSimpleFilter::const_iterator itFilter = filter.begin(); itFilter != filter.end(); ++itFilter )
		{
			bool bInnerChecked = true;
			for ( CSimpleFilterItem::const_iterator itFilterItem = itFilter->begin(); itFilterItem != itFilter->end(); ++itFilterItem )
			{
				if ( szFolder.find( *itFilterItem ) == string::npos )
				{
					bInnerChecked = false;
					break;
				}
			}
			if ( bInnerChecked )
			{
				bChecked = true;
				break;
			}
		}
	}
	else
	{
		bChecked = true;
	}
	if ( nState >= 0 && nState < states.size() )
	{
		if ( !states[nState] )
		{
			bChecked = false;
		}
	}
	if ( bTranslated && !_bTranslated )
	{
		bChecked = false;
	}
	return bChecked;
}


int SSimpleFilter::operator&( IXmlSaver &xs )
{
	xs.Add( "Filter", &filter );
	xs.Add( "Translated", &bTranslated );
	xs.Add( "States", &states );
	return 0;
}


int SMainFrameParams::INVALID_FILTER_NUMBER = -1;
	

int SMainFrameParams::SSearchParam::operator&( IXmlSaver &xs )
{
	xs.Add( "FindDown", &bFindDown );
	xs.Add( "FindMatchCase", &bFindMatchCase );
	xs.Add( "FindWholeWord", &bFindWholeWord );
	xs.Add( "FindText", &strFindText );
	xs.Add( "WindowType", &nWindowType );
	xs.Add( "Position", &nPosition );
	return 0;
}


int SMainFrameParams::operator&( IXmlSaver &xs )
{
	xs.Add( "ZIPToolPath", &szZIPToolPath );
	xs.Add( "LastOpenedELKName", &szLastOpenedELKName );
	xs.Add( "LastOpenedPAKName", &szLastOpenedPAKName );
	xs.Add( "LastOpenedXLSName", &szLastOpenedXLSName );
	xs.Add( "RecentList", &recentList );
	xs.Add( "PreviousPath", &szPreviousPath );
	xs.Add( "LastPath", &szLastPath );
	xs.Add( "LastELKElement", &nLastELKElement );
	xs.Add( "FilterMap", &filterMap );
	xs.Add( "CurrentFilter", &szCurrentFilterName );
	xs.Add( "CollapseItem", &bCollapseItem );
	xs.Add( "CodePage", &nCodePage );
	xs.Add( "FullScreen", &bFullScreen );
	xs.Add( "Rect", &rect );
	xs.Add( "GameFolder", &szGameFolder );
	xs.Add( "HelpFilePath", &szHelpFilePath );
	xs.Add( "CurrentFolder", &szCurrentFolder );
	return 0;
}


void SMainFrameParams::ValidatePath( string *pszPath, bool bFolder )
{
	if ( ( pszPath->size() < 2 ) || ( ( *pszPath )[1] != ':' ) )
	{
		if ( !pszPath->empty() )
		{
			if ( ( *pszPath )[0] == '\\' )
			{
				( *pszPath ) = pszPath->substr( 1 );
			}
		}
		( *pszPath ) = szCurrentFolder + ( *pszPath );
	}
	if ( bFolder )
	{
		if ( ( *pszPath )[pszPath->size() - 1] != '\\' )
		{
			( *pszPath ) += string( "\\" );	
		}
	}
	else
	{
		if ( ( *pszPath )[pszPath->size() - 1] == '\\' )
		{
			( *pszPath ) = pszPath->substr( 0, pszPath->size() - 1 );
		}
	}
	NStr::ToLower( pszPath );
}


void SMainFrameParams::LoadFromRegistry( const string &rszRegistryKey, bool bShortApperence )
{
	CString strKey;
	string szFormat;
	int nValue = 0;
	CRegistrySection registrySection( HKEY_CURRENT_USER, KEY_READ, rszRegistryKey.c_str() );
	
	{
		char pBuffer[0xFFF + 1];
		::GetCurrentDirectory( 0xFFF, pBuffer );
		szCurrentFolder = string( pBuffer ) + string( "\\" );
	}

	//strKey.LoadString( IDS_REGISTRY_KEY_ZIP_TOOL_PATH );
	//registrySection.LoadString( strKey, &szZIPToolPath, "" );
	//if ( szZIPToolPath.empty() )
	szZIPToolPath = szCurrentFolder + string( CELK::ZIP_EXE );
	szHelpFilePath = szCurrentFolder + string( CELK::ELK_CHM );

	strKey.LoadString( IDS_REGISTRY_KEY_LAST_OPENED_ELK_NAME );
	registrySection.LoadString( strKey, &szLastOpenedELKName, "" );

	strKey.LoadString( IDS_REGISTRY_KEY_LAST_OPENED_PAK_NAME );
	registrySection.LoadString( strKey, &szLastOpenedPAKName, "" );

	strKey.LoadString( IDS_REGISTRY_KEY_LAST_OPENED_XLS_NAME );
	registrySection.LoadString( strKey, &szLastOpenedXLSName, "" );

	nValue = 1;
	strKey.LoadString( IDS_REGISTRY_KEY_COLLAPSE_ITEM );
	registrySection.LoadNumber( strKey, _T( "%d" ), &nValue, 1 );
	bCollapseItem = ( nValue  > 0 );

	strKey.LoadString( IDS_REGISTRY_LAST_PATH );
	registrySection.LoadString( strKey, &szLastPath, "" );
	
	strKey.LoadString( IDS_REGISTRY_LAST_ELK_ELEMENT );
	registrySection.LoadNumber( strKey, _T( "%d" ), &nLastELKElement, 0 );

	nValue = 0;
	strKey.LoadString( IDS_REGISTRY_KEY_FULL_SCREEN );
	registrySection.LoadNumber( strKey, _T( "%d" ), &nValue, 0 );
	bFullScreen = ( nValue  > 0 );

	strKey.LoadString( IDS_REGISTRY_KEY_RECT );
	registrySection.LoadRect( strKey, _T( "%d" ), &rect, CTRect<int>( 0, 0, 0, 0 ) );

	//strKey.LoadString( IDS_REGISTRY_KEY_CODE_PAGE );
	//registrySection.LoadNumber( strKey, _T( "%d" ), &nCodePage, CELK::DEFAULT_CODE_PAGE );
	//if ( nCodePage == CELK::DEFAULT_CODE_PAGE )
	{
		nCodePage = ::GetACP();
	}
	
	int nRecentCount = 0;
	strKey.LoadString( IDS_REGISTRY_KEY_RECENT_LIST );
	szFormat = StrFmt( _T( "%ss" ), LPCTSTR( strKey ) );
	registrySection.LoadNumber( szFormat.c_str(), _T( "%d" ), &nRecentCount, 0 );
	recentList.clear();
	for ( int nRecentIndex = 0; nRecentIndex < nRecentCount; ++nRecentIndex )
	{
		list<string>::iterator posRecent = recentList.insert( recentList.end(), "" );
		string szFormat = StrFmt( _T( "%s%d" ), LPCTSTR( strKey ), nRecentIndex );
		registrySection.LoadString( szFormat.c_str(), &( *posRecent ), "" );
	}

	strKey.LoadString( IDS_REGISTRY_CURRENT_FILTER_NAME );
	registrySection.LoadString( strKey, &szCurrentFilterName, "" );
	{
		CRegistrySection registrySection( HKEY_LOCAL_MACHINE, KEY_READ, CELK::GAME_REGISTRY_FOLDER );
		registrySection.LoadString( CELK::GAME_REGISTRY_KEY, &szGameFolder, "" );
		CStringManager::CutFileExtention( &szGameFolder, CELK::GAME_FILE_NAME );
		if ( ( !szGameFolder.empty() ) && ( szGameFolder[szGameFolder.size() - 1] != '\\' ) )
		{
			szGameFolder += "\\";
		}
	}

	//if ( !bShortApperence )
	{
		int nFilterCount = 0;
		strKey.LoadString( IDS_REGISTRY_FILTER );
		szFormat = StrFmt( _T( "%ss" ), LPCTSTR( strKey ) );
		registrySection.LoadNumber( szFormat.c_str(), _T( "%d" ), &nFilterCount, 0 );
		filterMap.clear();
		if ( nFilterCount > 0 )
		{
			for ( int nFilterIndex = 0; nFilterIndex < nFilterCount; ++nFilterIndex )
			{
				string szFilterName;
				szFormat = StrFmt( _T( "%s%d_Name" ), LPCTSTR( strKey ), nFilterIndex );
				registrySection.LoadString( szFormat.c_str(), &szFilterName, "" );

				if ( !szFilterName.empty() )
				{
					SSimpleFilter &rSimpleFilter = filterMap[szFilterName];
					rSimpleFilter.states.resize(SELKTextProperty::STATE_COUNT, true );
				
					szFormat = StrFmt( _T( "%s%d_%s" ), LPCTSTR( strKey ), nFilterIndex, SELKTextProperty::STATE_LABELS[SELKTextProperty::STATE_NOT_TRANSLATED] );
					registrySection.LoadNumber( szFormat.c_str(), _T( "%d" ), &( rSimpleFilter.states[SELKTextProperty::STATE_NOT_TRANSLATED] ), 1 );

					szFormat = StrFmt( _T( "%s%d_%s" ), LPCTSTR( strKey ), nFilterIndex, SELKTextProperty::STATE_LABELS[SELKTextProperty::STATE_OUTDATED] );
					registrySection.LoadNumber( szFormat.c_str(), _T( "%d" ), &( rSimpleFilter.states[SELKTextProperty::STATE_OUTDATED] ), 1 );

					szFormat = StrFmt( _T( "%s%d_%s" ), LPCTSTR( strKey ), nFilterIndex, SELKTextProperty::STATE_LABELS[SELKTextProperty::STATE_TRANSLATED] );
					registrySection.LoadNumber( szFormat.c_str(), _T( "%d" ), &( rSimpleFilter.states[SELKTextProperty::STATE_TRANSLATED] ), 1 );

					szFormat = StrFmt( _T( "%s%d_%s" ), LPCTSTR( strKey ), nFilterIndex, SELKTextProperty::STATE_LABELS[SELKTextProperty::STATE_APPROVED] );
					registrySection.LoadNumber( szFormat.c_str(), _T( "%d" ), &( rSimpleFilter.states[SELKTextProperty::STATE_APPROVED] ), 1 );

					nValue = 0;
					szFormat = StrFmt( _T( "%s%d_Changed" ), LPCTSTR( strKey ), nFilterIndex );
					registrySection.LoadNumber( szFormat.c_str(), _T( "%d" ), &nValue, 0 );
					rSimpleFilter.bTranslated = ( nValue > 0 );

					int nFilterItemIndex = 0;
					while ( true )
					{
						int nFilterItemStringIndex = 0;
						CSimpleFilterItem filterItem;
						while( true )
						{
							szFormat = StrFmt( _T( "%s%d_%d_%d" ), LPCTSTR( strKey ), nFilterIndex, nFilterItemIndex, nFilterItemStringIndex );
							string szFilterItemString;
							registrySection.LoadString( szFormat.c_str(), &szFilterItemString, "" );
							if ( szFilterItemString.empty() )
							{
								break;
							}
							filterItem.push_back( szFilterItemString );
							++nFilterItemStringIndex;
						}
						if ( nFilterItemStringIndex == 0 )
						{
							break;
						}
						rSimpleFilter.filter.push_back( filterItem );
						++nFilterItemIndex;
					}
				}
			}
		}
	}
	if ( filterMap.empty() )
	{
		filterMap.clear();
		{
			SSimpleFilter &rSimpleFilter = filterMap[_T( "All" )];
			rSimpleFilter.states.resize( SELKTextProperty::STATE_COUNT, 1 );
		}
		{
			SSimpleFilter &rSimpleFilter = filterMap[_T( "Not Translated" )];
			rSimpleFilter.states.resize( SELKTextProperty::STATE_COUNT, 0 );
			rSimpleFilter.states[SELKTextProperty::STATE_NOT_TRANSLATED] = 1;
			rSimpleFilter.states[SELKTextProperty::STATE_OUTDATED] = 0;
			rSimpleFilter.states[SELKTextProperty::STATE_TRANSLATED] = 0;
			rSimpleFilter.states[SELKTextProperty::STATE_APPROVED] = 0;
		}
		{
			SSimpleFilter &rSimpleFilter = filterMap[_T( "Outdated" )];
			rSimpleFilter.states.resize( SELKTextProperty::STATE_COUNT, 0 );
			rSimpleFilter.states[SELKTextProperty::STATE_NOT_TRANSLATED] = 0;
			rSimpleFilter.states[SELKTextProperty::STATE_OUTDATED] = 1;
			rSimpleFilter.states[SELKTextProperty::STATE_TRANSLATED] = 0;
			rSimpleFilter.states[SELKTextProperty::STATE_APPROVED] = 0;
		}
		{
			SSimpleFilter &rSimpleFilter = filterMap[_T( "Translated" )];
			rSimpleFilter.states.resize( SELKTextProperty::STATE_COUNT, 0 );
			rSimpleFilter.states[SELKTextProperty::STATE_NOT_TRANSLATED] = 0;
			rSimpleFilter.states[SELKTextProperty::STATE_OUTDATED] = 0;
			rSimpleFilter.states[SELKTextProperty::STATE_TRANSLATED] = 1;
			rSimpleFilter.states[SELKTextProperty::STATE_APPROVED] = 0;
		}
		{
			SSimpleFilter &rSimpleFilter = filterMap[_T( "Approved" )];
			rSimpleFilter.states.resize( SELKTextProperty::STATE_COUNT, 0 );
			rSimpleFilter.states[SELKTextProperty::STATE_NOT_TRANSLATED] = 0;
			rSimpleFilter.states[SELKTextProperty::STATE_OUTDATED] = 0;
			rSimpleFilter.states[SELKTextProperty::STATE_TRANSLATED] = 0;
			rSimpleFilter.states[SELKTextProperty::STATE_APPROVED] = 1;
		}
		szCurrentFilterName = _T( "All" );
	}

	//
	nValue = 1;
	strKey.LoadString( IDS_REGISTRY_KEY_FIND_DOWN );
	registrySection.LoadNumber( strKey, _T( "%d" ), &nValue, 1 );
	searchParam.bFindDown = ( nValue  > 0 );

	nValue = 0;
	strKey.LoadString( IDS_REGISTRY_KEY_FIND_MATCH_CASE );
	registrySection.LoadNumber( strKey, _T( "%d" ), &nValue, 0 );
	searchParam.bFindMatchCase = ( nValue  > 0 );

	nValue = 0;
	strKey.LoadString( IDS_REGISTRY_KEY_FIND_WHOLE_WORD );
	registrySection.LoadNumber( strKey, _T( "%d" ), &nValue, 0 );
	searchParam.bFindWholeWord = ( nValue  > 0 );

	strKey.LoadString( IDS_REGISTRY_KEY_FIND_WINDOW );
	registrySection.LoadNumber( strKey, _T( "%d" ), &( searchParam.nWindowType ), (int)( SSearchParam::WT_ORIGINAL ) );

	strKey.LoadString( IDS_REGISTRY_KEY_FIND_POSITION );
	registrySection.LoadNumber( strKey, _T( "%d" ), &( searchParam.nPosition ), 0 );
}


void SMainFrameParams::SaveToRegistry( const string &rszRegistryKey, bool bShortApperence )
{
	CString strKey;
	string szFormat;
	::RegDeleteKey( HKEY_CURRENT_USER, rszRegistryKey.c_str() );
	CRegistrySection registrySection( HKEY_CURRENT_USER, KEY_WRITE, rszRegistryKey.c_str() );

	//strKey.LoadString( IDS_REGISTRY_KEY_ZIP_TOOL_PATH );
	//registrySection.SaveString( strKey, szZIPToolPath );

	strKey.LoadString( IDS_REGISTRY_KEY_LAST_OPENED_ELK_NAME );
	registrySection.SaveString( strKey, szLastOpenedELKName );

	strKey.LoadString( IDS_REGISTRY_KEY_LAST_OPENED_PAK_NAME );
	registrySection.SaveString( strKey, szLastOpenedPAKName );

	strKey.LoadString( IDS_REGISTRY_KEY_LAST_OPENED_XLS_NAME );
	registrySection.SaveString( strKey, szLastOpenedXLSName );
	
	strKey.LoadString( IDS_REGISTRY_KEY_COLLAPSE_ITEM );
	registrySection.SaveNumber( strKey, _T( "%d" ), bCollapseItem );

	strKey.LoadString( IDS_REGISTRY_LAST_PATH );
	registrySection.SaveString( strKey, szLastPath );

	strKey.LoadString( IDS_REGISTRY_LAST_ELK_ELEMENT );
	registrySection.SaveNumber( strKey, _T( "%d" ), nLastELKElement );

	strKey.LoadString( IDS_REGISTRY_KEY_FULL_SCREEN );
	registrySection.SaveNumber( strKey, _T( "%d" ), bFullScreen );

	strKey.LoadString( IDS_REGISTRY_KEY_RECT );
	registrySection.SaveRect( strKey, _T( "%d" ), rect );

	strKey.LoadString( IDS_REGISTRY_KEY_CODE_PAGE );
	registrySection.SaveNumber( strKey, _T( "%d" ), nCodePage );

	int nRecentCount = recentList.size();
	strKey.LoadString( IDS_REGISTRY_KEY_RECENT_LIST );
	szFormat = StrFmt( _T( "%ss" ), LPCTSTR( strKey ) );
	registrySection.SaveNumber( szFormat.c_str(), _T( "%d" ), nRecentCount );
	int nRecentIndex = 0;
	for ( list<string>::const_iterator itRecent = recentList.begin(); itRecent != recentList.end(); ++itRecent )
	{
		string szFormat = StrFmt( _T( "%s%d" ), LPCTSTR( strKey ), nRecentIndex );
		registrySection.SaveString( szFormat.c_str(), ( *itRecent ) );
		++nRecentIndex;
	}

	//if ( !bShortApperence )
	{
		strKey.LoadString( IDS_REGISTRY_CURRENT_FILTER_NAME );
		registrySection.SaveString( strKey, szCurrentFilterName );

		int nFilterCount = filterMap.size();
		strKey.LoadString( IDS_REGISTRY_FILTER );
		szFormat = StrFmt( _T( "%ss" ), LPCTSTR( strKey ) );
		registrySection.SaveNumber( szFormat.c_str(), _T( "%d" ), nFilterCount );
	
		int nFilterIndex = 0;
		for ( CFilterMap::const_iterator itFilter = filterMap.begin(); itFilter != filterMap.end(); ++itFilter )
		{
			szFormat = StrFmt( _T( "%s%d_Name" ), LPCTSTR( strKey ), nFilterIndex );
			registrySection.SaveString( szFormat.c_str(), itFilter->first );

			szFormat = StrFmt( _T( "%s%d_%s" ), LPCTSTR( strKey ), nFilterIndex, SELKTextProperty::STATE_LABELS[SELKTextProperty::STATE_NOT_TRANSLATED] );
			registrySection.SaveNumber( szFormat.c_str(), _T( "%d" ), itFilter->second.states[SELKTextProperty::STATE_NOT_TRANSLATED] );

			szFormat = StrFmt( _T( "%s%d_%s" ), LPCTSTR( strKey ), nFilterIndex, SELKTextProperty::STATE_LABELS[SELKTextProperty::STATE_OUTDATED] );
			registrySection.SaveNumber( szFormat.c_str(), _T( "%d" ), itFilter->second.states[SELKTextProperty::STATE_OUTDATED] );

			szFormat = StrFmt( _T( "%s%d_%s" ), LPCTSTR( strKey ), nFilterIndex, SELKTextProperty::STATE_LABELS[SELKTextProperty::STATE_TRANSLATED] );
			registrySection.SaveNumber( szFormat.c_str(), ( "%d" ), itFilter->second.states[SELKTextProperty::STATE_TRANSLATED] );

			szFormat = StrFmt( _T( "%s%d_%s" ), LPCTSTR( strKey ), nFilterIndex, SELKTextProperty::STATE_LABELS[SELKTextProperty::STATE_APPROVED] );
			registrySection.SaveNumber( szFormat.c_str(), _T( "%d" ), itFilter->second.states[SELKTextProperty::STATE_APPROVED] );
			
			szFormat = StrFmt( _T( "%s%d_Changed" ), LPCTSTR( strKey ), nFilterIndex );
			registrySection.SaveNumber( szFormat.c_str(), _T( "%d" ), itFilter->second.bTranslated );

			int nFilterItemIndex = 0;
			for ( CSimpleFilter::const_iterator itSimpleFilter = itFilter->second.filter.begin(); itSimpleFilter != itFilter->second.filter.end(); ++itSimpleFilter )
			{
				int nFilterItemStringIndex = 0;
				for ( CSimpleFilterItem::const_iterator itFilterItem = itSimpleFilter->begin(); itFilterItem != itSimpleFilter->end(); ++itFilterItem )
				{
					szFormat = StrFmt( _T( "%s%d_%d_%d" ), LPCTSTR( strKey ), nFilterIndex, nFilterItemIndex, nFilterItemStringIndex );
					registrySection.SaveString( szFormat.c_str(), ( *itFilterItem ) );
					++nFilterItemStringIndex;
				}
				++nFilterItemIndex;
			}
			++nFilterIndex;
		}
	}

	//
	strKey.LoadString( IDS_REGISTRY_KEY_FIND_DOWN );
	registrySection.SaveNumber( strKey, _T( "%d" ), searchParam.bFindDown );

	strKey.LoadString( IDS_REGISTRY_KEY_FIND_MATCH_CASE );
	registrySection.SaveNumber( strKey, _T( "%d" ), searchParam.bFindMatchCase );

	strKey.LoadString( IDS_REGISTRY_KEY_FIND_WHOLE_WORD );
	registrySection.SaveNumber( strKey, _T( "%d" ), searchParam.bFindWholeWord );

	strKey.LoadString( IDS_REGISTRY_KEY_FIND_WINDOW );
	registrySection.SaveNumber( strKey, _T( "%d" ), searchParam.nWindowType );

	strKey.LoadString( IDS_REGISTRY_KEY_FIND_POSITION );
	registrySection.SaveNumber( strKey, _T( "%d" ), searchParam.nPosition );
}


const SSimpleFilter* SMainFrameParams::GetCurrentFilter() const
{
	if ( !szCurrentFilterName.empty() )
	{
		CFilterMap::const_iterator itFilter = filterMap.find( szCurrentFilterName );
		if ( itFilter != filterMap.end() )
		{
			return &( itFilter->second );
		}
	}
	return 0;
}

