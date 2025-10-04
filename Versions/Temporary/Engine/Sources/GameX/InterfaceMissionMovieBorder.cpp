#include "stdafx.h"
#include "InterfaceMissionMovieBorder.h"
#include "GameXClassIDs.h"

// CInterfaceMissionMovieBorder

CInterfaceMissionMovieBorder::CInterfaceMissionMovieBorder() : 
	CInterfaceScreenBase( "MissionMovieBorder", "no_section" )
{
}

CInterfaceMissionMovieBorder::~CInterfaceMissionMovieBorder()
{
}

bool CInterfaceMissionMovieBorder::Init()
{
	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	RegisterObservers();
	
	AddScreen( this );

	return true;
}

void CInterfaceMissionMovieBorder::RegisterObservers()
{
	AddObserver( "quicksave", &CInterfaceMissionMovieBorder::MsgQuickSave );
	AddObserver( "quickload", &CInterfaceMissionMovieBorder::MsgQuickLoad );
}

void CInterfaceMissionMovieBorder::MsgQuickSave( const SGameMessage &msg )
{
}

void CInterfaceMissionMovieBorder::MsgQuickLoad( const SGameMessage &msg )
{
}

void CInterfaceMissionMovieBorder::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );
}

bool CInterfaceMissionMovieBorder::StepLocal( bool bAppActive )
{
	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return bAppActive;
}

bool CInterfaceMissionMovieBorder::Execute( const string &szSender, const string &szReaction )
{
	return false;
}

int CInterfaceMissionMovieBorder::Check( const string &szCheckName ) const
{
	return 0;
}

bool CInterfaceMissionMovieBorder::ProcessEvent( const SGameMessage &msg )
{
	if ( CInterfaceScreenBase::ProcessEvent( msg ) )
		return true;

	return false;
}

void CInterfaceMissionMovieBorder::AfterLoad()
{
	CInterfaceScreenBase::AfterLoad();

	RegisterObservers();
}

// CICMissionMovieBorder

CICMissionMovieBorder::CICMissionMovieBorder()
{
}

void CICMissionMovieBorder::PreCreate()
{
}

void CICMissionMovieBorder::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
}

void CICMissionMovieBorder::Configure( const char *pszConfig )
{
}

REGISTER_SAVELOAD_CLASS( 0x17149C80, CInterfaceMissionMovieBorder );
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MISSION_BORDER, CICMissionMovieBorder );


