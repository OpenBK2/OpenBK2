// Reaction.cpp: implementation of the CReaction class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MessageReaction.h"


#include "UIInternal.h"

//#include "..\Input\Input.h"

#include "WindowTabControl.h"
// здесь генерируется отдельный ID, который потом прописывается в в файле include_?????????????.h
REGISTER_SAVELOAD_CLASS(0x11075CC0, CARSetGlobalVar)
REGISTER_SAVELOAD_CLASS(0x11075CC1, CARRemoveGlobalVar)
REGISTER_SAVELOAD_CLASS(0x11075CC3, CMessageReactionB2)
REGISTER_SAVELOAD_CLASS(0x11075CC2, CARSendUIMessage)
REGISTER_SAVELOAD_CLASS(0x15083400, CARSendGameMessage)
REGISTER_SAVELOAD_CLASS(0x15084341, CARSiwtchTab)

//////////////////////////////////////////////////////////////////////
BASIC_REGISTER_CLASS(IMessageReactionB2);
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//CMessageReactionB2
//////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
bool CMessageReactionB2::Execute( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const
{
	bool bRes = true;
	if ( !commonBefore.empty() )
	{
		//DebugTrace( "\t\t\tCOMMON_BEFORE\n" );
		bRes &= Execute( &commonBefore, pScreen, pScript, pProg, wKeyboardFlags );
	}
	int nCustomCheckReturn = 0;
	
	if ( pCheck )
	{
		nCustomCheckReturn = pCheck->Check( pScreen, pScript, pProg, wKeyboardFlags );

		CMessageSequences::const_iterator it = branches.find( nCustomCheckReturn );
		//DebugTrace( "\t\t\tCustomCheck \t%d \n", nCustomCheckReturn );
		NI_ASSERT( it != branches.end(), StrFmt( "branche \"%d\" not found", nCustomCheckReturn ) );

		if ( !it->second.empty() )
			bRes &= Execute( &it->second, pScreen, pScript, pProg, wKeyboardFlags );
	}

	if ( !commonAfter.empty() )
	{
		//DebugTrace( "\t\t\tCOMMON_AFTER\n" );
		bRes &= Execute( &commonAfter, pScreen, pScript, pProg, wKeyboardFlags );
	}
	return bRes;
}
//////////////////////////////////////////////////////////////////////
int CMessageReactionB2::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pCheck );
	saver.Add( 2, &branches );
	saver.Add( 3, &commonBefore );					// always run and before any branch
	saver.Add( 4, &commonAfter );						// always run and after any branch
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMessageReactionB2::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SMessageReactionComplex *pDesc = checked_cast<const NDb::SMessageReactionComplex*>( _pDesc );

	for ( vector<NDb::SMessageSequienceEntry>::const_iterator it = pDesc->branches.begin(); it != pDesc->branches.end(); ++it )
		InitSequienceByDesc( &branches[it->nCustomCheckReturn], it->sequience.data );
	
	pCheck = CUIFactory::MakeCheck( pDesc->pConditionCheck );
	
	
	InitSequienceByDesc( &commonBefore, pDesc->commonBefore.data );
	InitSequienceByDesc( &commonAfter, pDesc->commonAfter.data );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMessageReactionB2::InitSequienceByDesc( CMessageSequence *pCreate, const CMessageSequienceDesc &src )
{
	pCreate->resize( src.size() );
	int i = 0;
	for ( CMessageSequienceDesc::const_iterator it = src.begin(); it != src.end(); ++it )
	{
		(*pCreate)[i] = CUIFactory::MakeReaction( *it );
		++i;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CMessageReactionB2::Execute( const CMessageSequence *pToExecute, interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const
{
	if ( pToExecute->empty() ) 
		return false;
	//execute other sections
	for ( CMessageSequence::const_iterator reaction = pToExecute->begin(); reaction != pToExecute->end(); ++reaction )
	{
		if ( !(*reaction)->Execute( pScreen, pScript, pProg, wKeyboardFlags ) )
			return false;
	}
	return true;
}
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//		CARSetGlobalVar
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
bool CARSetGlobalVar::Execute( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const
{  
#if !defined(_FINALRELEASE) && !defined(_BETARELEASE)
	DebugTrace( "\t\t SetGlobalVar \tvarName =\t\"%s\", \tValue =\t\"%s\"\n", 
										pDesc->szVarName.c_str(), 
										pDesc->szVarValue.c_str() );
#endif // #if !defined(_FINALRELEASE) && !defined(_BETARELEASE)
	NGlobal::SetVar( pDesc->szVarName.c_str(), pDesc->szVarValue.c_str() ); 
	return true;
}
//////////////////////////////////////////////////////////////////////
int CARSetGlobalVar::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pDesc );
	return 0;
}
//////////////////////////////////////////////////////////////////////
void CARSetGlobalVar::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	pDesc = checked_cast<const NDb::SARSetGlobalVar*>( _pDesc );
}
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//		CARRemoveGlobalVar
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
bool CARRemoveGlobalVar::Execute( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const
{  
#if !defined(_FINALRELEASE) && !defined(_BETARELEASE)
	DebugTrace( "\t\t RemoveGlabalVar \tvarName =\t\"%s\"\n", 
										pDesc->szVarName.c_str() );
#endif // #if !defined(_FINALRELEASE) && !defined(_BETARELEASE)
	NGlobal::RemoveVar( pDesc->szVarName ); 
	return true;
}	
//////////////////////////////////////////////////////////////////////
int CARRemoveGlobalVar::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pDesc );
	return 0;
}
//////////////////////////////////////////////////////////////////////
void CARRemoveGlobalVar::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	pDesc = checked_cast<const NDb::SARRemoveGlobalVar*>( _pDesc );
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//		CARSendUIMessage
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
bool CARSendUIMessage::Execute( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const
{
//#if !defined(_FINALRELEASE) && !defined(_BETARELEASE)
//	DebugTrace( "\t\t SendMessage\tID =\t\"%s\tParam =\t%s\n", 
//										pDesc->szMessageID.c_str(), pDesc->szStringParam.c_str() );
//#endif // #if !defined(_FINALRELEASE) && !defined(_BETARELEASE)
	
	return dynamic_cast<CWindow*>( pScreen )->ProcessMessage( 
		SBUIMessage(pDesc->szMessageID, pDesc->szStringParam, pDesc->nIntParam) );
}
//////////////////////////////////////////////////////////////////////
int CARSendUIMessage::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pDesc );
	return 0;
}
//////////////////////////////////////////////////////////////////////
void CARSendUIMessage::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	pDesc = checked_cast<const NDb::SARSendUIMessage*>( _pDesc );
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//		CARSendGameMessage
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
bool CARSendGameMessage::Execute( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const
{
	NInput::PostEvent( pDesc->szEventName, pDesc->nIntParam, 0 );
	//IGameEvent *pEvent = Input()->GetEvent( pDesc->szEventName );
	//pEvent->RaiseEvent( pDesc->nIntParam, 0 );
	return true;
}
//////////////////////////////////////////////////////////////////////
int CARSendGameMessage::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pDesc );
	return 0;
}
//////////////////////////////////////////////////////////////////////
void CARSendGameMessage::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	pDesc = checked_cast<const NDb::SARSendGameMessage*>( _pDesc );
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//		CARSwitchTab
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
bool CARSiwtchTab::Execute( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const
{
	CWindowTabControl *pTab = dynamic_cast<CWindowTabControl *>(pScreen->GetElement( pDesc->szTabControlName, true ));
	NI_ASSERT( pTab != 0, StrFmt( "tab control with name \"%s\" not found", pDesc->szTabControlName ) );
	pTab->SetActive( pDesc->nTab );
	return true;
}
//////////////////////////////////////////////////////////////////////
int CARSiwtchTab::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pDesc );
	return 0;
}
//////////////////////////////////////////////////////////////////////
void CARSiwtchTab::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	pDesc = checked_cast<const NDb::SARSwitchTab*>( _pDesc );
}
//////////////////////////////////////////////////////////////////////
