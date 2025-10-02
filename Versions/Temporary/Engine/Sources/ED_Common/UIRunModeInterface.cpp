#include "StdAfx.h"

#include "ui/commandparam.h"
#include "ui/dbuserinterface.h"
#include "ui/ui.h"
#include "input/gamemessage.h"
#include "ui/uifactory.h"
#include "UIRunModeInterface.h"
#include "UIScene.h"

CUIRunModeInterface::CUIRunModeInterface( const string &rszTypeName, const CDBID &rDBID, const CDBID &rTemplateWindowDBID, const CDBID &rTemplateScreenDBID )
{
	LoadWindow( rszTypeName, rDBID, rTemplateWindowDBID, rTemplateScreenDBID );
}


CUIRunModeInterface::~CUIRunModeInterface()
{
	if ( pScreen ) 
	{
		Singleton<IUIScene>()->RemoveWindow( pScreen );
		pScreen = 0;
	}
}


void CUIRunModeInterface::Step( bool bAppActive )
{
	CRunModeInterfaceBase::Step( bAppActive );
	if ( bAppActive )
	{
		if ( pScreen )
			pScreen->Segment( 10 );
	}
}

bool CUIRunModeInterface::ProcessEvent( const struct SGameMessage &msg )
{
	if ( !CRunModeInterfaceBase::ProcessEvent( msg ) )
	{
		if ( pScreen )
			return pScreen->ProcessEvent( msg );
	}
	return false;
}


void CUIRunModeInterface::LoadWindow( const string &rszTypeName, const CDBID &rDBID, const CDBID &rTemplateWindowDBID, const CDBID &rTemplateScreenDBID )
{
	const NDb::SUIGameConsts *pUI = 0;//pGameRoot->pConsts->pUI;
	//const NDb::SGameRoot *pGameRoot = NDb::Get<NDb::SGameRoot>( GetGlobalVar("game_root", 2) );
	//Singleton<IUIInitialization>()->SetUIConsts( pUI );

	//const int nWindowSimpleID = NGlobal::GetVar( "uied_winid", 0 );
	//const int nWindowScreenID = NGlobal::GetVar( "uied_scrid", 0 );
	NI_ASSERT( !rTemplateWindowDBID.IsEmpty(), "CUIRunModeInterface::LoadWindow: templateWindowDBID is empty" );
	NI_ASSERT( !rTemplateScreenDBID.IsEmpty(), "CUIRunModeInterface::LoadWindow: templateScreenDBID is empty" );

	if ( rszTypeName == "WindowSimpleShared" )
	{
		CDBPtr<NDb::SWindowScreen> pScreenTemplate = NDb::Get<NDb::SWindowScreen>( rTemplateScreenDBID );
		
		pScreen = Singleton<IUIInitialization>()->CreateScreenFromDesc( pScreenTemplate, pUI, 0, Singleton<IUIScene>()->GetG2DView(), 0, 0 );

		// avoid fitting UI to viewport size
		Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( 1024, 768 );
		
		CDBPtr<NDb::SWindowSimple> pTemplate = NDb::Get<NDb::SWindowSimple>( rTemplateWindowDBID );
		CDBPtr<NDb::SWindowSimpleShared> pShared = NDb::Get<NDb::SWindowSimpleShared>( rDBID );
		
		CPtr<NDb::SWindowSimple> pInstance = pTemplate->Duplicate();
		pInstance->pShared = pShared;

		CPtr<IWindow> pMainWindow = Singleton<IUIInitialization>()->CreateWindowFromDesc( pInstance );
		pScreen->AddChild( pMainWindow, true );
	}
	else if ( rszTypeName == "WindowScreenShared" )
	{
		CDBPtr<NDb::SWindowScreenShared> pShared = NDb::Get<NDb::SWindowScreenShared>( rDBID );
		CDBPtr<NDb::SWindowScreen> pTemplate = NDb::Get<NDb::SWindowScreen>( rTemplateScreenDBID );
		
		CPtr<NDb::SWindowScreen> pInstance = pTemplate->Duplicate();
		pInstance->pShared = pShared;

		pScreen = Singleton<IUIInitialization>()->CreateScreenFromDesc(
			pInstance, pUI, 0,
			Singleton<IUIScene>()->GetG2DView(), 0, 0 );
	}
	else 
	{
		NI_ASSERT( 0, "CUIRunModeInterface::LoadWindow: Unknown widget type" );
	}

	if ( pScreen )
	{
		Singleton<IUIScene>()->AddWindow( pScreen );
		pScreen->RegisterObservers();
	}

	// avoid fitting UI to window size
	Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( 1024, 768 );
}



