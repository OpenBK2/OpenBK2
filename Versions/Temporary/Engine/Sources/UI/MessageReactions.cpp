// MessageReactions.cpp: implementation of the CMessageReactions class.
//


#include "stdafx.h"
#include "../script/script.h"
#include "MessageReactions.h"
#include "../System/VFSOperations.h"

//CRAP{ FOR TEST
#include "MessageReaction.h"
//CRAP}



// ************************************************************************************************************************ //
// **
// ** helper fucntions to get/set global vars
// **
// **
// **
// ************************************************************************************************************************ //

int ScriptErrorOut( struct lua_State *state )
{
	Script script( state );
	Script::Object obj = script.GetObject(script.GetTop());
	const string szError = StrFmt( "Script error: %s", obj.GetString() );
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, szError.c_str(), 0xffff0000, true );
	DebugTrace( "%s\n", szError.c_str() );
	return 0;
}

static int Sqrt( struct lua_State *pState )
{
	Script script( pState );
	script.PushNumber(  sqrt( (double)script.GetObject(1) ) );
	return 1;
}

static int OutputStringValue( struct lua_State *state )
{
	Script script(state);
	NI_ASSERT( script.GetTop() == 2, "Script function must have 2 arguments on the stack" );			//два аргумента
	string szStr = script.GetObject( -2 );
	int nValue = script.GetObject( -1 );
	DebugTrace( "****Debug LUA script: %s %s\n", szStr.c_str(), nValue );
	return 0;
}

SRegFunction reglist[] =
{
	{ "_ERRORMESSAGE"			,	ScriptErrorOut			},
	{ "OutputStringValue"	, OutputStringValue		},
	//
	{ 0, 0 },
};

void MessageReactionsRegisterScriptFunctions()
{
	NScript::AddScriptFunctionsToSaveLoad( reglist );
}


// CMessageReactions::


void CMessageReactions::InitByDesc( const NDb::SMessageReactionsDesc &desc )
{
	for ( vector<NDb::SReactionSequenceEntry>::const_iterator it = desc.reactions.begin();
																										it != desc.reactions.end();
																										++it )
	{
		reactions[it->szName] = CUIFactory::MakeReaction( it->pReaction );
	}
	if ( !desc.szScriptFileRef.empty() )
		InitScript( desc.szScriptFileRef );
}

void CMessageReactions::RunScriptText( const string &szScriptBody )
{
	pScript = CreateScriptWrapper();
	pScript->Init();
	pScript->AddRegFunctions( reglist );

	if ( pScript->RunScript( szScriptBody.c_str() ) != 0 ) 
		pScript = 0;
}
void CMessageReactions::InitScript( const string &szScriptFileName )
{
	pScript = 0;
	if ( !szScriptFileName.empty() )
	{
		string szScriptText;
		CFileStream stream( NVFS::GetMainVFS(), szScriptFileName );
		if ( stream.IsOk() )
		{
			const int nSize = stream.GetSize();
			if ( nSize > 0 )
			{
				szScriptText.resize( stream.GetSize() );
				stream.Read( const_cast<char*>(szScriptText.data()), nSize );
			}
		}
		RunScriptText( szScriptText );
	}
}

int CMessageReactions::operator&( IBinSaver &saver )
{
	saver.Add( 1, &reactions );
	saver.Add( 2, &pScript );
	return 0;
}

bool CMessageReactions::Execute( const string &szSender, const string &szReactionKey, struct IScreen *pScreen, struct IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags )
{
	if ( pProg && 
		(pProg->NeedFlags() ? pProg->Execute( szSender, szReactionKey, wKeyboardFlags ) : pProg->Execute( szSender, szReactionKey )  ) ) return true;
		
	CReactions::iterator it = reactions.find( szReactionKey );
	NI_ASSERT( it != reactions.end(), StrFmt( "unregistered reaction \"%s\"", szReactionKey.c_str() ) );
	if ( it != reactions.end() )
		return it->second->Execute( pScreen, pScript, pProg, wKeyboardFlags );
	return false;
}

bool CMessageReactions::Execute( const string &szSender, const NDb::SUIDesc *pReaction, struct IScreen *pScreen, struct IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags )
{
	CObj<IMessageReactionB2> pReactionB2 = CUIFactory::MakeReaction( pReaction );
	return pReactionB2->Execute( pScreen, pScript, pProg, wKeyboardFlags );
}


void CMessageReactions::Register( const string &szReactionKey, IMessageReactionB2 *pReaction )
{
	reactions[szReactionKey] = pReaction;
}

