#include "StdAfx.h"
#include "ui/commandparam.h"
#include "ui/dbuserinterface.h"
#include "misc/2darray.h"
#include "stats_b2_m1/iconsset.h"
#include "InterfaceMPWaitPlayers.h"
#include "GameXClassIDs.h"
#include "MultiplayerCommandManager.h"
#include "Misc/StrProc.h"

#include <zconf.h>

// CInterfaceMPWaitPlayers

CInterfaceMPWaitPlayers::CInterfaceMPWaitPlayers() : 
	CInterfaceMPScreenBase( "MPWaitPlayers", "mp_wait_players" )
{
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_WAITING_INFO, SMPUILagInfoMessage, &CInterfaceMPWaitPlayers::OnLagInfoMessage );
}

bool CInterfaceMPWaitPlayers::Init()
{
	if ( CInterfaceMPScreenBase::Init() == false )
		return false;
	RegisterObservers();
	AddScreen( this );
	pResumeButton = GetChildChecked<IButton>( GetScreen(), "ResumeButton", true );
	if ( pResumeButton )
		pResumeButton->ShowWindow( false );
	bOwnLag = false;
	pOwnTimeLabel = GetChildChecked<ITextView>( GetScreen(), "LabelTimeLeft", true );
	pOwnTime = GetChildChecked<ITextView>( GetScreen(), "TimeLeft", true );
	pPlayerList = GetChildChecked<IScrollableContainer>( GetScreen(), "PlayerList", true );
	if ( pPlayerList )
		pPlayerList->ShowWindow( false );
	pPlayerListItem = GetChildChecked<IWindow>( pPlayerList, "PlayerListItem", true );
	if ( pPlayerListItem )
		pPlayerListItem->ShowWindow( false );
	bOwnLag = false;

	return true;
}

void CInterfaceMPWaitPlayers::RegisterObservers()
{
	AddObserver( "multiplayer_pause", &CInterfaceMPWaitPlayers::MsgOnMultiplayerPause );
}

bool CInterfaceMPWaitPlayers::Execute( const string &szSender, const string &szReaction )
{
	if ( szReaction == "react_interrupt" )
	{
		if ( bOwnLag )
			Singleton<IMPToUIManager>()->AddUIMessage( EMUI_MP_PAUSE );
		return true;
	}

	return false;
}

int CInterfaceMPWaitPlayers::Check( const string &szCheckName ) const
{
	return 0;
}

void CInterfaceMPWaitPlayers::MsgOnMultiplayerPause( const SGameMessage &msg )
{
	Singleton<IMPToUIManager>()->AddUIMessage( EMUI_MP_PAUSE );
}

bool CInterfaceMPWaitPlayers::OnLagInfoMessage( SMPUILagInfoMessage *pMsg )
{
	bOwnLag = pMsg->bOwnLag;
	if ( pResumeButton )
		pResumeButton->ShowWindow( bOwnLag );
	if ( pOwnTimeLabel )
		pOwnTimeLabel->ShowWindow( bOwnLag );
	if ( pOwnTime )
		pOwnTime->ShowWindow( bOwnLag );
	if ( pPlayerList )
		pPlayerList->ShowWindow( !bOwnLag );

	if ( bOwnLag )
	{
		wstring wszTime = NStr::ToUnicode( StrFmt( "%d:%02d", pMsg->nOwnTimeLeft / 60, pMsg->nOwnTimeLeft % 60 ) );
		if ( pOwnTime )
			pOwnTime->SetText( pOwnTime->GetDBText() + wszTime );
	}
	else if ( pPlayerList )
	{
		pPlayerList->RemoveItems();
		for ( SMPUILagInfoMessage::CLaggerList::iterator it = pMsg->lags.begin(); it != pMsg->lags.end(); ++it )
		{
			SMPUILagInfoMessage::SLagItem &lagItem = (*it);
			AddPlayerLine( lagItem.szName, lagItem.nSecondsLeft );
		}

		pPlayerList->Update();
	}

	return true;
}

void CInterfaceMPWaitPlayers::AddPlayerLine( const string &szName, const int nTime )
{
	if ( !pPlayerListItem )
		return;

	IWindow *pItemWnd = AddWindowCopy( pPlayerList, pPlayerListItem );
	pItemWnd->ShowWindow( true );
	ITextView *pName = GetChildChecked<ITextView>( pItemWnd, "ItemName", true );
	ITextView *pTime = GetChildChecked<ITextView>( pItemWnd, "ItemTime", true );

	if ( pName )
		pName->SetText( pName->GetDBText() + NStr::ToUnicode( szName ) );
	if ( pTime )
	{
		if ( nTime >= 0 )
		{
			wstring wszTime = NStr::ToUnicode( StrFmt( "%d:%02d", nTime / 60, nTime % 60 ) );
			pTime->SetText( pTime->GetDBText() + wszTime );
		}
	}
	pPlayerList->PushBack( pItemWnd, false );
}

// CICMPWaitPlayers

void CICMPWaitPlayers::PreCreate()
{
}

void CICMPWaitPlayers::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICMPWaitPlayers::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( 0x1713CBC1, CInterfaceMPWaitPlayers );
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MP_WAIT_PLAYERS, CICMPWaitPlayers );


