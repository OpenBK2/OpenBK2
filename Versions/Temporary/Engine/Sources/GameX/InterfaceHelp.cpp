#include "stdafx.h"
#include "InterfaceHelp.h"
#include "GameXClassIDs.h"
#include "InterfaceState.h"
#include "System/Text.h"
#include "SceneB2/Cursor.h"

// CInterfaceHelp

CInterfaceHelp::CInterfaceHelp() : 
	CInterfaceScreenBase( "Help", "help" )
{
}

CInterfaceHelp::~CInterfaceHelp()
{
}

bool CInterfaceHelp::Init()
{
	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	RegisterObservers();
	
	AddScreen( this );

	return true;
}

void CInterfaceHelp::MakeInterior( const std::wstring &wszHeader, const std::wstring &wszDesc )
{
	pMain = GetChildChecked<IWindow>( pScreen, "Main", true );
	pHeader = GetChildChecked<ITextView>( pMain, "Header", true );
	pDescCont = GetChildChecked<IScrollableContainer>( pMain, "DescCont", true );
	pDesc = GetChildChecked<ITextView>( pMain, "Desc", true );
	
	if ( pHeader )
		pHeader->SetText( pHeader->GetDBText() + wszHeader );
	if ( pDescCont && pDesc )
	{
		pDescCont->PushBack( dynamic_cast_ptr<IWindow*>( pDesc ), false );
		pDesc->SetText( pDesc->GetDBText() + wszDesc ); 
		pDescCont->Update();
	}
}

void CInterfaceHelp::RegisterObservers()
{
}

void CInterfaceHelp::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );
	
	if ( bFocus )
	{
		Cursor()->Show( true );
		Cursor()->SetMode( NDb::USER_ACTION_UNKNOWN );
	}
}

bool CInterfaceHelp::Execute( const std::string &szSender, const std::string &szReaction )
{
	if ( szReaction == "close" )
		return OnCloseReaction( szSender );
		
	return false;
}

int CInterfaceHelp::Check( const std::string &szCheckName ) const
{
	return 0;
}

bool CInterfaceHelp::OnCloseReaction( const std::string &szSender )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
	
	return true;
}

bool CInterfaceHelp::ProcessEvent( const SGameMessage &msg )
{
	return CInterfaceScreenBase::ProcessEvent( msg );
}

bool CInterfaceHelp::StepLocal( bool bAppActive )
{
	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return CInterfaceScreenBase::StepLocal( bAppActive );
}

void CInterfaceHelp::AfterLoad()
{
	CInterfaceScreenBase::AfterLoad();

	RegisterObservers();
}

// CICHelp

void CICHelp::PreCreate()
{
}

void CICHelp::PostCreate( IInterface *pInterface )
{
	const NDb::SUIScreenEntry *pEntry = InterfaceState()->GetScreenEntry( szInterfaceType );
	if ( pEntry && (CHECK_TEXT_NOT_EMPTY_PRE(pEntry->,HelpHeader) || CHECK_TEXT_NOT_EMPTY_PRE(pEntry->,HelpDesc)))
	{
		std::wstring szHeader;
		std::wstring szDesc;
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pEntry->,HelpHeader) )
			szHeader = GET_TEXT_PRE(pEntry->,HelpHeader);
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pEntry->,HelpDesc) )
			szDesc = GET_TEXT_PRE(pEntry->,HelpDesc);
		pInterface->MakeInterior( szHeader, szDesc );
	}
	NMainLoop::PushInterface( pInterface );
}

void CICHelp::Configure( const char *pszConfig )
{
	szInterfaceType = pszConfig;
}

REGISTER_SAVELOAD_CLASS( 0x17136B01, CInterfaceHelp );
REGISTER_SAVELOAD_CLASS( ML_COMMAND_HELP_SCREEN, CICHelp );


