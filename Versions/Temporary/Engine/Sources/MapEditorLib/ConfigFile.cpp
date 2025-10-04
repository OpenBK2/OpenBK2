#include "stdafx.h"

#include "Misc/StrProc.h"
#include "ConfigFile.h"

const char SConfigFile::DIVIDERS[] = " \t";


void SConfigFile::SConfigEntry::Load( const string &rszLine )
{
	szLine = rszLine;
	NStr::TrimBoth( szLine, "\n\r" );
	//
	szKeyword.clear();
	szParams.clear();
	//
	if ( !rszLine.empty() )
	{
	  if ( ( szLine.substr( 0, 1 ).compare( ";" ) != 0 ) &&
				 ( szLine.substr( 0, 2 ).compare( "//" ) != 0 ) )  
		{
			string szLocalLine = szLine;
			NStr::TrimBoth( szLocalLine, DIVIDERS );
			const int nPos = szLocalLine.find_first_of( DIVIDERS );
			if ( nPos != string::npos )
			{
				szKeyword = szLocalLine.substr( 0, nPos );
				szParams = szLocalLine.substr( nPos );
				NStr::TrimBoth( szKeyword, DIVIDERS );
				NStr::TrimBoth( szParams, DIVIDERS );
			}
			else
			{
				szKeyword = szLocalLine;
			}
		}
	}
}


int SConfigFile::Load( const string &rszFileName )
{
	Clear();
	//
  string szBuffer;
	CFileStream stream( rszFileName, CFileStream::WIN_READ_ONLY );
  const int nSize = stream.GetSize();
  szBuffer.resize( nSize );
  stream.Read( &( szBuffer[0] ), nSize );
	//
	vector<string> lineList;
	NStr::SplitString( szBuffer.c_str(), &lineList, '\n' );
	for ( vector<string>::const_iterator itLine = lineList.begin(); itLine != lineList.end(); ++itLine )
	{
		CConfigEntryList::iterator itConfigEntry = configEntryList.insert( configEntryList.end() );
		itConfigEntry->Load( *itLine );
	}
	return lineList.size();
}


void SConfigFile::Save( const string &rszFileName )
{
  string szBuffer;
	for ( CConfigEntryList::const_iterator itConfigEntry = configEntryList.begin(); itConfigEntry != configEntryList.end(); ++itConfigEntry )
	{
		szBuffer += itConfigEntry->szLine + "\r\n";
	}
	//
	CFileStream stream( rszFileName, CFileStream::WIN_CREATE );
  stream.Write( &( szBuffer[0] ), szBuffer.size() );
}


bool SConfigFile::GetParams( CParamsList *pParamsList, const string &rszKeyword, bool bIgnoreCase )
{
	string szKeyword = rszKeyword;
	if ( bIgnoreCase )
	{
		NStr::ToLower( &szKeyword );
	}
	//
	bool bFound = false;
	for ( CConfigEntryList::const_iterator itConfigEntry = configEntryList.begin(); itConfigEntry != configEntryList.end(); ++itConfigEntry )
	{
		string szConfigEntryKeyword = itConfigEntry->szKeyword;
		if ( bIgnoreCase )
		{
			NStr::ToLower( &szConfigEntryKeyword );
		}
		if ( szConfigEntryKeyword == szKeyword )
		{
			bFound = true;	
			if ( pParamsList != 0 )
			{
				pParamsList->push_back( itConfigEntry->szParams );
			}
			else
			{
				break;
			}
		}
	}
	return bFound;
}


void SConfigFile::AddLine( const string &rszLine )
{
	CConfigEntryList::iterator itConfigEntry = configEntryList.insert( configEntryList.end() );
	itConfigEntry->Load( rszLine );
}


void SConfigFile::AddKeyword( const string &rszKeyword, const string &rszParams )
{
	CConfigEntryList::iterator itConfigEntry = configEntryList.insert( configEntryList.end() );
	itConfigEntry->szLine = rszKeyword + string( " " ) + rszParams;
	itConfigEntry->szKeyword = rszKeyword;
	itConfigEntry->szParams = rszParams;
}


int SConfigFile::RemoveKeyword( const string &rszKeyword, bool bIgnoreCase )
{
	string szKeyword = rszKeyword;
	if ( bIgnoreCase )
	{
		NStr::ToLower( &szKeyword );
	}
	//
	int nErasedCount = 0;
	for ( CConfigEntryList::iterator itConfigEntry = configEntryList.begin(); itConfigEntry != configEntryList.end(); )
	{
		string szConfigEntryKeyword = itConfigEntry->szKeyword;
		if ( bIgnoreCase )
		{
			NStr::ToLower( &szConfigEntryKeyword );
		}
		if ( szConfigEntryKeyword == szKeyword )
		{
			itConfigEntry = configEntryList.erase( itConfigEntry );
			++nErasedCount;
		}
		else
		{
			++itConfigEntry;
		}
	}
	return nErasedCount;
}


// basement storage  


