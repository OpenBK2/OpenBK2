
#pragma once

#include "Input/gamemessage.h"
#include "Main/MainLoop.h"
#include "UISpecificB2/UISpecificB2.h"

namespace NGScene
{
	class CRTPtr;
}

class CInterfaceScreenBase : public IInterfaceBase, protected NInput::CGMORegContainer
{
	NInput::CGMORegContainer importantMsgs;
	const std::string szInterfaceType;		// interface type - "InterMission", "Mission", etc.
	const std::string szBindSection;			// this interface bind section
	//
	NTimer::STime nTime;
	CObj<class CWindowTextView> pVersionWindow;
	bool bInFocus;
	CVec2 vLastScreenSize;
	bool bShowScreenOnGetFocus;
	std::unordered_map<int,bool> registeredIDsForMLHandler;

	// Is interface transparent and we should draw under it
	bool bIsTransparent;

private:
	bool MsgLButtonDown( const SGameMessage &msg );
	bool MsgLButtonUp( const SGameMessage &msg );
	bool MsgLButtonDblClk( const SGameMessage &msg );
	bool MsgRButtonDown( const SGameMessage &msg );
	bool MsgRButtonUp( const SGameMessage &msg );
	bool MsgRButtonDblClk( const SGameMessage &msg );
	bool MsgMouseMove( const struct SGameMessage &msg );
	bool MsgHelpScreen( const struct SGameMessage &msg );

	bool OnShowConsole( const struct SGameMessage &msg );
	void SetVersionWindowAfterLoad();
	void FillVersionWindow();
protected:
	CObj<IScreen> pScreen;

protected:
	bool IsInFocus() const { return bInFocus; }
	// Выводит экран помощи при выполнении всех условий показа
	bool CheckedShowHelpScreen( bool bForced );
	// 
	virtual bool IsShowHelpScreenOnInit() { return true; }
	//
	bool ChangeResolution();
	//
	virtual bool StepLocal( bool bAppActive ) { return bAppActive; }
	// update statistics
	void ShowVersionInfo();

	virtual void OnMouseMove( const CVec2 &vPos );
	virtual void OnButtonDown( const CVec2 &vPos, int nButton );
	virtual void OnButtonUp( const CVec2 &vPos, int nButton );
	virtual void OnButtonDblClk( const CVec2 &vPos, int nButton );

	bool ProcessEvent( const struct SGameMessage &msg );

	// Пауза вне игры, обусловленная интерфейсом
	void PauseIntermission( bool bPause );
	
	const std::string& GetBindSection() const { return szBindSection; }
	virtual void RestoreBindSection();
	
	void AddScreen( struct IProgrammedReactionsAndChecks *pReactions );
	IScreen* GetScreen() { return pScreen; }
	
	template <typename TObj, typename TMsg>
		void AddImportantObserver( const std::string &szMsgName, bool (TObj::*_pfnMemFun)( const TMsg &_msg ) )
	{
		importantMsgs.AddObserver( szMsgName, _pfnMemFun );
	}
	
	void SetDynamicTextView( ITextView *pView, const std::vector< std::pair<std::wstring, std::wstring> > &params );
	void SetDynamicTooltip( IWindow *pWnd, const std::wstring &wszTooltip, const std::vector< std::pair<std::wstring, std::wstring> > &params );

	void SetMainWindowTexture( IWindow *pMainWindow, const NDb::STexture *pTexture );
	
	// disable explicit destruction
	~CInterfaceScreenBase();
public:
	CInterfaceScreenBase( const std::string &_szInterfaceType, const std::string &_szBindSection );
	//
	virtual bool Init();
	virtual bool Init( struct ITransceiver *pTransceiver ) { return CInterfaceScreenBase::Init(); }

	void Step( bool bAppActive );
	bool ShouldLimitFrameRate() const;
	virtual void OnGetFocus( bool bFocus );
	virtual void StartInterface();
	virtual int operator&( IBinSaver &saver );
	bool IsTransparent() const { return bIsTransparent; }

	// call this function after load is complete
	virtual void AfterLoad();
	virtual void Draw( NGScene::CRTPtr *pTexture = 0 );
	
	const std::string& GetInterfaceType() const { return szInterfaceType; }

	virtual void HideUnfocusedScreen();
};

template <class TInterfaceScreen>
class CInterfaceCommandBase : public IInterfaceCommand
{
protected:
	typedef TInterfaceScreen IInterface;
	//
	CInterfaceCommandBase() {  }
	virtual ~CInterfaceCommandBase() {  }
	//
	virtual void PreCreate() { NMainLoop::ResetStack(); }
	virtual void PostCreate( IInterface *pInterface ) { NMainLoop::PushInterface( pInterface ); }
public:
	void Exec()
	{
		PreCreate();
		//
		IInterface *pInterface = new IInterface();
		pInterface->Init();
		pInterface->StartInterface();
		//
		PostCreate( pInterface );
	}
};

bool AddUIScreen( IScreen *pWindowScreen, const std::string &szScreenEntryName, IProgrammedReactionsAndChecks *pReactions );

#define INTERFACE_COMMAND_DECLARE( CommandName, InterfaceName )		\
class CommandName : public CInterfaceCommandBase< InterfaceName >	\
{																																	\
	OBJECT_NOCOPY_METHODS( CommandName );														\
};


