#include "StdAfx.h"
#include "uifactory.h"

#include "IMessageReaction.h"
#include "..\3Dmotor\RectLayout.h"
#include "UIScreen.h"
#include "UIML.h"
#include "DBUIConsts.h"


CDBPtr<NDb::SUIGameConsts> NUIFactory::pConsts;
extern CVec2 vScreenRect;
extern CVec2 vScreenOrg;
IScreen * CUIFactory::pScreenDuringLoad = 0;
NInput::CGMORegContainer CUIFactory::registeredMessages;
bool OnKnownMessage( const struct SGameMessage &msg )
{
	return true;
}

CVirtualScreenController::CVirtualScreenController()
{
	// by default
	//::vScreenRect.Set( 1024, 768 );
	//::vScreenOrg.Set( 0, 0 );
}

void CVirtualScreenController::GetResolution( int *nSizeX, int *nSizeY )
{
	*nSizeX = ::vScreenRect.x;
	*nSizeY = ::vScreenRect.y;
}

void CVirtualScreenController::SetResolution( int nSizeX, int nSizeY )
{
	::vScreenRect.Set( nSizeX, nSizeY );
}

void CVirtualScreenController::SetOrigin( int nOrgX, int nOrgY )
{
	::vScreenOrg.Set( nOrgX, nOrgY );
}

void CVirtualScreenController::GetOrigin( int *pOrgX, int *pOrgY )
{
	*pOrgX = ::vScreenOrg.x;
	*pOrgY = ::vScreenOrg.y;
}

void CVirtualScreenController::VirtualToScreen( const CTRect<float> &src, CTRect<float> *pRes )
{
	::VirtualToScreen( src, pRes );
}

void CVirtualScreenController::VirtualToScreen( CRectLayout *pRects )
{
	::VirtualToScreen( pRects );
}

void CVirtualScreenController::ScreenToVirtual( const CVec2 &vPos, CVec2 *pScreenPos )
{
	::ScreenToVirtual( vPos, pScreenPos );
}

void MessageReactionsRegisterScriptFunctions();

CUIFactory::CUIFactory()
{
	pVirtualScreenController = new CVirtualScreenController();
//	MessageReactionsRegisterScriptFunctions();
}

void CUIFactory::SetMLHandler( const wstring &wsTAG, interface IMLHandler *pHandler )
{
	customMLHandlers[wsTAG] = pHandler;
}

void CUIFactory::RegisterMLHandlersInternal( interface IML *pML )
{
	for ( CCustomMLHandlersMap::iterator it = customMLHandlers.begin(); it != customMLHandlers.end(); ++it )
	{
		pML->SetHandler( it->first, it->second );
	}
}

void CUIFactory::RegisterMLHandlers( interface IML *pML )
{
	Singleton<CUIFactory>()->RegisterMLHandlersInternal( pML );
}

IWindow * CUIFactory::CreateWindowFromDesc( const struct NDb::SUIDesc *pDesc )
{ 
	return CUIFactory::MakeWindow( pDesc );
}

IWindow * CUIFactory::CreateScreenFromDesc( const struct NDb::SUIDesc *_pDesc, 
																						const NDb::SUIGameConsts *_pConsts,
																						IProgrammedReactionsAndChecks *pReactionsAndChecks,
																						NGScene::I2DGameView * p2DView, 
																						NGScene::IGameView *pGView, NGScene::IGameView *pInterface3DView )
{
	NUIFactory::pConsts = _pConsts;
	IScreen *pScreen = MakeObjectVirtual<CWindowScreen>( UI_SCREEN );
	// commented to allow in-game & in-editor correct work
	//pScr->SetGView( p2DView, pGView, pInterface3DView );
	pScreen->Load( _pDesc, pReactionsAndChecks );
	return pScreen;
}

void CUIFactory::Set2DGameView( NGScene::I2DGameView* pView )
{
	CWindowScreen::Set2DGView( pView );
}

/*
NGScene::I2DGameView * CUIFactory::Get2DGameView()
{
	return CWindowScreen::Get2DGameView();
}
*/

CWindow * CUIFactory::MakeWindow( const struct NDb::SUIDesc *pDesc )
{
	if ( !pDesc ) 
		return 0;

	NI_ASSERT( checked_cast<const NDb::SWindow*>(pDesc)->pShared != 0, StrFmt( "shared part is empty, WindowName \"%s\"", checked_cast<const NDb::SWindow*>(pDesc)->szName.c_str() ) );
	CWindow * p = MakeObjectVirtual<CWindow>( pDesc->nClassTypeID );

	p->InitByDesc( pDesc );
	return p;
}

IWindowPart * CUIFactory::MakeWindowPart( const struct NDb::SUIDesc *pDesc )
{
	if ( !pDesc ) return 0;
	IWindowPart * p = MakeObject<IWindowPart>( pDesc->nClassTypeID );
	p->InitByDesc( pDesc );
	p->Init();
	return p;
}

IMessageReactionB2 * CUIFactory::MakeReaction( const struct NDb::SUIDesc *pDesc )
{
	if ( !pDesc ) 
		return 0;
	static int nCurrentID = 0;
	
	NI_ASSERT( nCurrentID != pDesc->nClassTypeID, StrFmt( "attempt to create cyclic Reactions DBID = \"%s\"", pDesc->GetDBID().ToString().c_str() ) );
	nCurrentID = pDesc->nClassTypeID;
	
	IMessageReactionB2 *p = MakeObject<IMessageReactionB2>( pDesc->nClassTypeID );
	p->InitByDesc( pDesc );
	
	nCurrentID = 0;
	
	return p;
}

IMessageCheck * CUIFactory::MakeCheck( const struct NDb::SUIDesc *pDesc )
{
	if ( !pDesc ) return 0;
	IMessageCheck * p = MakeObject<IMessageCheck>( pDesc->nClassTypeID );
	p->InitByDesc( pDesc );
	return p;
}

IUIEffector *CUIFactory::MakeEffect( const NDb::SUIStateBase *pCmd, interface IScreen *pScreen, SWindowContext *pContext, const string &szAnimateWindow )
{
	if ( !pCmd ) return 0;
	IUIEffector *p = MakeObject<IUIEffector>( pCmd->nClassTypeID );
	p->Configure( pCmd, pScreen, pContext, szAnimateWindow );
	return p;
}

void CUIFactory::RegisterMessage( const string &szMessage )
{
	registeredMessages.AddObserver( szMessage, OnKnownMessage );
}

bool CUIFactory::IsMessageRegistered( const SGameMessage &msg  )
{
	return registeredMessages.ProcessEvent( msg, 0 );
}

IUIInitialization* CreateUIInitialization()
{
	return new CUIFactory;
}

REGISTER_SAVELOAD_CLASS( 0x11069C80, SWindowContextCommon );
REGISTER_SAVELOAD_CLASS( 0x1109BC00, CUIFactory );
REGISTER_SAVELOAD_CLASS( 0x110B5CC0, CVirtualScreenController );

