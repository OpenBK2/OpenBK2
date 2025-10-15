// CustomCheck.cpp: implementation of the CCustomCheck class.
//

#include "stdafx.h"
#include "CustomCheck.h"
#include "Script/ScriptWrapper.h"

#include <cstdint>

REGISTER_SAVELOAD_CLASS(UI, 0x11075CC4,CCheckRunScript)
REGISTER_SAVELOAD_CLASS(UI, 0x11075CC5, CCheckPreprogrammed)
REGISTER_SAVELOAD_CLASS(UI, 0x15083383, CCheckIsWindowEnabled)
REGISTER_SAVELOAD_CLASS(UI, 0x110B5300, CCheckIsWindowVisible)
REGISTER_SAVELOAD_CLASS(UI, 0x170B6340, CCheckIsTabActive)


// CCheckRunScript::


int CCheckRunScript::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pDesc );
	return 0;
}

int CCheckRunScript::Check( struct IScreen *pScreen, struct IScriptWrapper *pScript, struct IProgrammedReactionsAndChecks *pProg, uint16_t wKeyboardFlags  ) const
{
	NI_ASSERT( pScript != 0, StrFmt( "CCheckRunScript function = \"%s\" but don't have script loaded", pDesc->szScriptFunction.c_str() ) );
	if ( pScript )
	{
		return pScript->CallScriptFunction( pDesc->szScriptFunction.c_str() );
	}
	return 0;
}

void CCheckRunScript::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	pDesc = checked_cast<const NDb::SCheckRunScript*>( _pDesc );
}


// CCheckPreprogrammed::


void CCheckPreprogrammed::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	pDesc = checked_cast<const NDb::SCheckPreprogrammed*>( _pDesc );
}
int CCheckPreprogrammed::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pDesc );
	return 0;
}
int CCheckPreprogrammed::Check( struct IScreen *pScreen, struct IScriptWrapper *pScript, struct IProgrammedReactionsAndChecks *pProg, uint16_t wKeyboardFlags  ) const
{
	NI_ASSERT( pProg != 0, StrFmt("try to call check \"%s\" without preprogrammed checks provided", pDesc->szCheckName.c_str()) );
	return ( pProg->NeedFlags() ? pProg->Check( pDesc->szCheckName, wKeyboardFlags ) : pProg->Check( pDesc->szCheckName ) );
}


// CCheckIsWindowEnabled::


void CCheckIsWindowEnabled::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	pDesc = checked_cast<const NDb::SCheckIsWindowEnabled*>( _pDesc );
}
int CCheckIsWindowEnabled::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pDesc );
	return 0;
}
int CCheckIsWindowEnabled::Check( struct IScreen *pScreen, struct IScriptWrapper *pScript, struct IProgrammedReactionsAndChecks *pProg, uint16_t wKeyboardFlags  ) const
{
	NI_ASSERT( pScreen != 0, "try to call window check without screen provided" );
	//DebugTrace( "Check window enabled/visible %s", pDesc->szWindowName.c_str() );
	IWindow *pWnd = 0;
	if ( !pDesc->szParentWindowName.empty() )
	{
		IWindow *pParentWnd = checked_cast<IWindow*>(pScreen->GetVisibleElement( pDesc->szParentWindowName, true ));
		if ( pParentWnd )
			pWnd = checked_cast<IWindow*>(pParentWnd->GetVisibleChild( pDesc->szWindowName, true ));
	}
	if ( !pWnd )
		pWnd = checked_cast<IWindow*>(pScreen->GetVisibleElement( pDesc->szWindowName, true ));
	if ( !pWnd || !(pWnd->IsEnabled()) || !(pWnd->IsVisible()) )
		return 0;
	else
		return 1;
}


// CCheckIsWindowVisible ::


int CCheckIsWindowVisible::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pDesc );
	return 0;
}

int CCheckIsWindowVisible::Check( struct IScreen *pScreen, struct IScriptWrapper *pScript, struct IProgrammedReactionsAndChecks *pProg, uint16_t wKeyboardFlags  ) const
{
	NI_ASSERT( pScreen != 0, "try to call window check without screen provided" );
	//DebugTrace( "Check window visible %s", pDesc->szWindowName.c_str() );
	IWindow *pWnd = 0;
	if ( !pDesc->szParentWindowName.empty() )
	{
		IWindow *pParentWnd = checked_cast<IWindow*>(pScreen->GetVisibleElement( pDesc->szParentWindowName, true ));
		if ( pParentWnd )
			pWnd = checked_cast<IWindow*>(pParentWnd->GetVisibleChild( pDesc->szWindowName, true ));
	}
	if ( !pWnd )
		pWnd = checked_cast<IWindow*>(pScreen->GetVisibleElement( pDesc->szWindowName, true ));
	if ( !pWnd || !(pWnd->IsVisible()) )
		return 0;
	else
		return 1;
}

void CCheckIsWindowVisible::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	pDesc = checked_cast<const NDb::SCheckIsWindowVisible*>( _pDesc );
}



// CCheckIsTabActive ::


int CCheckIsTabActive::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pDesc );
	return 0;
}

int CCheckIsTabActive::Check( struct IScreen *pScreen, struct IScriptWrapper *pScript, struct IProgrammedReactionsAndChecks *pProg, uint16_t wKeyboardFlags  ) const
{
	NI_ASSERT( pScreen != 0, "try to call window check without screen provided" );
	DebugTrace( "Check tab active %s (%d)", pDesc->szTabControlName.c_str(), pDesc->nTab );
	IWindow *pWnd = checked_cast<IWindow*>(pScreen->GetVisibleElement( pDesc->szTabControlName, true ));
	if ( !pWnd || !(pWnd->IsVisible()) )
		return 0;
	ITabControl *pTab = dynamic_cast<ITabControl*>(pScreen->GetElement( pDesc->szTabControlName, true ));
	NI_ASSERT( pTab != 0, StrFmt( "tab control with name \"%s\" not found", pDesc->szTabControlName ) );
	if ( pTab->GetActive() != pDesc->nTab )
		return 0;
	return 1;
}

void CCheckIsTabActive::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	pDesc = checked_cast<const NDb::SCheckIsTabActive*>( _pDesc );
}


