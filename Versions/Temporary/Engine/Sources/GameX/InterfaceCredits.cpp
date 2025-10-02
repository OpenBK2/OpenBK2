#include "StdAfx.h"
#include "../ui/commandparam.h"
#include "../ui/dbuserinterface.h"
#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../stats_b2_m1/iconsset.h"
#include "./interfacecredits.h"
#include "../SceneB2/Scene.h"
#include "GameXClassIDs.h"
#include "../UI/SceneClassIDs.h"
#include "InterfaceState.h"


bool CInterfaceCredits::CReactions::Execute( const string &szSender, const string &szReaction )
{
	return false;
}

int CInterfaceCredits::CReactions::Check( const string &szCheckName ) const
{
	return 0;
}

CInterfaceCredits::~CInterfaceCredits()
{
	if ( pScreen ) 
		Scene()->RemoveScreen( pScreen );
}




bool CInterfaceCredits::Init()
{
	// load screen
	pScreen = MakeObjectVirtual<IScreen>( UI_SCREEN );
	pReactions = new CReactions( pScreen, this );
	if ( AddUIScreen(pScreen, "CreditsScreen", pReactions) == false )
		return false;
	pText = GetChildChecked<IWindow>( pScreen, "CreditsText", true );
	IScrollableContainer *pScroll = GetChildChecked<IScrollableContainer>( pScreen, "CreditsScroll", true );
		
	if ( pScroll && pText )
	{
		pText->ShowWindow( true );
		pScroll->ShowWindow( true );
		pScroll->PushBack( pText, false );
		int nStartPos;
		pScroll->GetPlacement( 0, 0, 0, &nStartPos );
		pText->SetPlacement( 0, nStartPos, 0, 0, EWPF_POS_Y );
	}

	timeLastMove = Singleton<IGameTimer>()->GetAbsTime();
	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	return true;
}

bool CInterfaceCredits::StepLocal( bool bAppActive )
{
	if ( !bAppActive ) 
		return false;

	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	if ( pText )
	{
		int nY, nSize;
		pText->GetPlacement( 0, &nY, 0, &nSize );
		const NTimer::STime curTime = Singleton<IGameTimer>()->GetAbsTime();
		int nTimeDelta = curTime - timeLastMove;
		if ( -nY == nSize )
		{
			if ( !bClosed )
			{
				NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "options_menu_end" );
				NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );
				bClosed = true;
			}
		}
		else
		{
			int nDelta = ( curTime - timeLastMove ) * 0.04f;
			if ( nDelta != 0 )
			{
				nY -= nDelta;
				timeLastMove = curTime;
				nY = Max( -nSize, nY );
				pText->SetPlacement( 0, nY, 0, 0, EWPF_POS_Y );
			}
		}
	}
	return CInterfaceScreenBase::StepLocal( bAppActive );
}

bool CInterfaceCredits::ProcessEvent( const SGameMessage &msg )
{
	if ( msg.mMessage.cType == NInput::CT_KEY )
	{
		if ( !bClosed )
		{
			NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "options_menu_end" );
			NMainLoop::Command( ML_COMMAND_MAIN_MENU, "" );
			bClosed = true;
		}
	}
	return CInterfaceScreenBase::ProcessEvent( msg );
}

// CICWinLooseDialog

void CICInterfaceCredist::PreCreate()
{
}

void CICInterfaceCredist::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICInterfaceCredist::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( 0x1119C382, CInterfaceCredits )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_CREDITS, CICInterfaceCredist )
REGISTER_SAVELOAD_CLASS_NM( 0x1119C381, CReactions, CInterfaceCredits );

