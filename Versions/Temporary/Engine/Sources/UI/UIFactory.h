#pragma once

#include "DBUIConsts.h"
#include "Input/GameMessage.h"
#include "UI.h"

#include "UI_export.h"

namespace NGScene
{
class I2DGameView;
}

namespace NDb
{
struct SUIGameConsts;
}

class CVirtualScreenController : public IVirtualScreenController
{
	OBJECT_NOCOPY_METHODS( CVirtualScreenController )
public:
	CVirtualScreenController();

	int operator&( IBinSaver &saver ) override
	{
		return 0;
	}

	//{ IVirtualScreenController
	void GetResolution( int *nSizeX, int *nSizeY ) override;
	void SetResolution( int nSizeX, int nSizeY ) override;
	void SetOrigin( int nOrgX, int nOrgY ) override;
	void GetOrigin( int *pOrgX, int *pOrgY ) override;
	void VirtualToScreen( const CTRect<float> &src, CTRect<float> *pRes ) override;
	void VirtualToScreen( class CRectLayout *pRects ) override;
	void ScreenToVirtual( const CVec2 &vPos, CVec2 *pScreenPos ) override;
	//}
};

namespace NUIFactory
{
EXTERNVAR CDBPtr<NDb::SUIGameConsts> pConsts;
}

class UI_EXPORT CUIFactory : public IUIInitialization
{
	OBJECT_BASIC_METHODS( CUIFactory )
	CObj<CVirtualScreenController> pVirtualScreenController;
	typedef std::unordered_map<std::wstring, CObj<IMLHandler>> CCustomMLHandlersMap;
	CCustomMLHandlersMap customMLHandlers;
	static IScreen *pScreenDuringLoad;

	static NInput::CGMORegContainer registeredMessages;

private:
	void RegisterMLHandlersInternal( struct IML *pML );

public:
	static void SetScreenDuringLoad( IScreen *_pScreen )
	{
		pScreenDuringLoad = _pScreen;
	}

	static IScreen *GetScreenDuringLoad()
	{
		return pScreenDuringLoad;
	}

	int operator&( IBinSaver &saver ) override
	{
		saver.Add( 1, &NUIFactory::pConsts );
		saver.Add( 2, &pVirtualScreenController );
		saver.Add( 3, &customMLHandlers );
		return 0;
	}

	CUIFactory();

	//{ IUIInitialization
	void SetUIConsts( const struct NDb::SUIGameConsts *_pConsts ) override
	{
		NUIFactory::pConsts = _pConsts;
	}

	void SetMLHandler( const std::wstring &wsTAG, struct IMLHandler *pHandler ) override;
	IWindow *CreateWindowFromDesc( const struct NDb::SUIDesc *pDesc ) override;
	IWindow *CreateScreenFromDesc( const struct NDb::SUIDesc *pDesc,
	                               const NDb::SUIGameConsts *pConsts,
	                               struct IProgrammedReactionsAndChecks *pReactionsAndChecks,
	                               NGScene::I2DGameView *p2DView,
	                               NGScene::IGameView *pGView,
	                               NGScene::IGameView *pInterface3DView ) override;

	IVirtualScreenController *GetVirtualScreenController() override
	{
		return pVirtualScreenController;
	}

	void Set2DGameView( NGScene::I2DGameView *pView ) override;

	// }

	static const NDb::SUIGameConsts *GetConsts()
	{
		return NUIFactory::pConsts;
	}

	static class CWindow *MakeWindow( const struct NDb::SUIDesc *pDesc );
	static struct IWindowPart *MakeWindowPart( const struct NDb::SUIDesc *pDesc );
	static struct IMessageReactionB2 *MakeReaction( const struct NDb::SUIDesc *pDesc );
	static struct IMessageCheck *MakeCheck( const struct NDb::SUIDesc *pDesc );
	static struct IUIEffector *MakeEffect( const NDb::SUIStateBase *pCmd,
	                                       struct IScreen *pScreen,
	                                       struct SWindowContext *pContext,
	                                       const std::string &szAnimateWindow );
	//static NGScene::I2DGameView * Get2DGameView();

	static void RegisterMLHandlers( struct IML *pML );

	static void RegisterMessage( const std::string &szMessage );
	static bool IsMessageRegistered( const SGameMessage &msg );
};
