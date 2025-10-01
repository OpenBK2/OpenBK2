#pragma once

#include "UI_export.h"

#include "UI.h"
#include "../Input/GameMessage.h"
#include "DBUIConsts.h"

namespace NGScene
{
	class I2DGameView;
}

namespace NDb
{
	struct SUIGameConsts;
}
//////////////////////////////////////////////////////////////////////
class CVirtualScreenController : public IVirtualScreenController
{
	OBJECT_NOCOPY_METHODS( CVirtualScreenController )
public:
	CVirtualScreenController();

	int operator&( IBinSaver &saver )
	{
		return 0;
	}
	//{ IVirtualScreenController
	void GetResolution( int *nSizeX, int *nSizeY );
	void SetResolution( int nSizeX, int nSizeY );
	void SetOrigin( int nOrgX, int nOrgY );
	void GetOrigin( int *pOrgX, int *pOrgY );
	void VirtualToScreen( const CTRect<float> &src, CTRect<float> *pRes );
	void VirtualToScreen( class CRectLayout *pRects );
	void ScreenToVirtual( const CVec2 &vPos, CVec2 *pScreenPos );
	//}
};
namespace NUIFactory
{
EXTERNVAR CDBPtr<NDb::SUIGameConsts> pConsts;
}
//////////////////////////////////////////////////////////////////////
class UI_EXPORT CUIFactory : public IUIInitialization
{
	OBJECT_BASIC_METHODS( CUIFactory )
	CObj<CVirtualScreenController> pVirtualScreenController;
	typedef hash_map< wstring, CObj<IMLHandler> > CCustomMLHandlersMap;
	CCustomMLHandlersMap customMLHandlers;
	static IScreen * pScreenDuringLoad;
	
	static NInput::CGMORegContainer registeredMessages;

private:
	void RegisterMLHandlersInternal( interface IML *pML );
public:
	static void SetScreenDuringLoad( IScreen *_pScreen ) { pScreenDuringLoad = _pScreen; }
	static IScreen * GetScreenDuringLoad() { return pScreenDuringLoad; }
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &NUIFactory::pConsts );
		saver.Add( 2, &pVirtualScreenController );
		saver.Add( 3, &customMLHandlers );
		return 0;
	}
	CUIFactory();
	//{ IUIInitialization
	void SetUIConsts( const struct NDb::SUIGameConsts *_pConsts ) { NUIFactory::pConsts = _pConsts; }
	void SetMLHandler( const wstring &wsTAG, interface IMLHandler *pHandler );
	IWindow * CreateWindowFromDesc( const struct NDb::SUIDesc *pDesc );
	IWindow * CreateScreenFromDesc( const struct NDb::SUIDesc *pDesc, 
																	const NDb::SUIGameConsts *pConsts, 
																	interface IProgrammedReactionsAndChecks *pReactionsAndChecks,
																	NGScene::I2DGameView * p2DView, 
																	NGScene::IGameView *pGView, NGScene::IGameView *pInterface3DView );
	IVirtualScreenController * GetVirtualScreenController() { return pVirtualScreenController; }
	virtual void Set2DGameView( NGScene::I2DGameView* pView );
	// }

	static const NDb::SUIGameConsts * GetConsts() { return NUIFactory::pConsts; }

	static class CWindow * MakeWindow( const struct NDb::SUIDesc *pDesc );
	static interface IWindowPart * MakeWindowPart( const struct NDb::SUIDesc *pDesc );
	static interface IMessageReactionB2 *MakeReaction( const struct NDb::SUIDesc *pDesc );
	static interface IMessageCheck *MakeCheck( const struct NDb::SUIDesc *pDesc );
	static interface IUIEffector *MakeEffect( const NDb::SUIStateBase *pCmd, interface IScreen *pScreen, struct SWindowContext *pContext, const string &szAnimateWindow  );
	//static NGScene::I2DGameView * Get2DGameView();

	static void RegisterMLHandlers( interface IML *pML );

	static void RegisterMessage( const string &szMessage );
	static bool IsMessageRegistered( const SGameMessage &msg );
};
//////////////////////////////////////////////////////////////////////
