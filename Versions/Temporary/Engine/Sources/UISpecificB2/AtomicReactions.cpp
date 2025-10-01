#include "StdAfx.h"

#include "atomicreactions.h"

#include "../Input/Bind.h"

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//		CARSetForcedAction
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
bool CARSetForcedAction::Execute( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const
{
	if ( NGlobal::GetVar( "m1", 0 ) == 0 )
		NInput::PostEvent( "set_forced_action", static_cast<int>(pDesc->eUserAction), 0 );
	else
		NInput::PostEvent( "set_forced_action", static_cast<int>(pDesc->eM1UserAction), 0 );
	//IGameEvent *pEvent = Input()->GetEvent( "set_forced_action" );
	//pEvent->RaiseEvent( static_cast<int>(pDesc->eUserAction), 0 );
	return true;
}
//////////////////////////////////////////////////////////////////////
int CARSetForcedAction::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pDesc );
	return 0;
}
//////////////////////////////////////////////////////////////////////
void CARSetForcedAction::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	pDesc = checked_cast<const NDb::SARSetForcedAction*>( _pDesc );
}
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//		CARSetSpecialAbility
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
bool CARSetSpecialAbility::Execute( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags ) const
{
	NInput::PostEvent( "set_special_ability", static_cast<int>(pDesc->eAbility), static_cast<int>(pDesc->eAbilityParam) );
	//IGameEvent *pEvent = Input()->GetEvent( "set_special_ability" );
	//pEvent->RaiseEvent( static_cast<int>(pDesc->eAbility), static_cast<int>(pDesc->eAbilityParam) );
	return true;
}
//////////////////////////////////////////////////////////////////////
int CARSetSpecialAbility::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pDesc );
	return 0;
}
//////////////////////////////////////////////////////////////////////
void CARSetSpecialAbility::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	pDesc = checked_cast<const NDb::SARSetSpecialAbility*>( _pDesc );
}
//////////////////////////////////////////////////////////////////////
int CRAPTooSmartCompilator_UISpecificB2_2();
int CRAPTooSmartCompilator_UISpecific()
{
	return CRAPTooSmartCompilator_UISpecificB2_2();
}
//////////////////////////////////////////////////////////////////////
REGISTER_SAVELOAD_CLASS(0x1508A301, CARSetForcedAction)
REGISTER_SAVELOAD_CLASS(0x1508D340, CARSetSpecialAbility)
