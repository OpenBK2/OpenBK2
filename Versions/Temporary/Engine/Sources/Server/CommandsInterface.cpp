#include "stdafx.h"

#include "CommandsInterface.h"
#include "Misc/StrProc.h"

const string SCommand::GetStr( const int nIndex ) const
{
	return params[nIndex];
}

const int SCommand::GetInt( const int nIndex ) const
{
	return NStr::ToInt( params[nIndex] );
}



bool CCommandsBase::GetCommand( SCommand *pCmd )
{ 
	if ( cmds.empty() )
		return false;

	*pCmd = cmds.front();
	cmds.pop_front();
	return true;
}

void CCommandsBase::PushCommand( const SCommand &cmd )
{ 
	cmds.push_back( cmd );
}

void CCommandsBase::PreprocessLine( string *pszLine, vector<string> *pszWords )
{
	NStr::TrimBoth( *pszLine );
	NStr::ReplaceAllChars( pszLine, '\n', ' ' );
	NStr::ReplaceAllChars( pszLine, '\t', ' ' );

	NStr::SplitString( *pszLine, pszWords, ' ' );
	vector<string>::iterator iter = pszWords->begin();
	while ( iter != pszWords->end() )
	{
		NStr::TrimBoth( *iter );
		if ( iter->empty() )
			iter = pszWords->erase( iter );
		else
			++iter;
	}

	NStr::ToLower( &(*pszWords)[0] );
}


