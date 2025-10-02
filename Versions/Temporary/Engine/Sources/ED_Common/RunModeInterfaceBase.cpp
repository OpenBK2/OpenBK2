#include "StdAfx.h"
#include "RunModeInterfaceBase.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"

CRunModeInterfaceBase::CRunModeInterfaceBase( CTimeCounter *_pTimer ) : 
	dwTime(0), pTimer(_pTimer)
{
	NInput::PurgeEvents();
}


CRunModeInterfaceBase::~CRunModeInterfaceBase()
{
	NInput::PurgeEvents();
}


void CRunModeInterfaceBase::Step( bool bAppActive )
{
	if ( !bAppActive )
	{
		if ( pTimer != 0 )
			pTimer->ResetTiming();
		return;
	}
	if ( pTimer != 0 )
		pTimer->Advance( 1.f, GetTime() );

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}

void CRunModeInterfaceBase::OnGetFocus( bool bFocus )
{
	if ( bFocus )
	{
		NInput::SetSection( "map_editor" );
		NInput::PurgeEvents();
	}
}

bool CRunModeInterfaceBase::ProcessEvent( const struct SGameMessage &msg )
{
	if( msg.mMessage.cType == NInput::CT_TIME )
	{
		dwTime = msg.mMessage.tTime;
	}
	return NInput::CGMORegContainer::ProcessEvent( msg, this );
}



