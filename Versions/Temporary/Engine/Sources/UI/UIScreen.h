#pragma once
#include "MessageReactions.h"
#include "System/FreeIDs.h"
#include "Tooltips.h"
#include "UIStates.h"
#include "Window.h"

#include <cstdint>

class CButtonGroup;

namespace NGScene
{
class I2DGameView;
}

struct STabPairCompare
{
	bool operator()( const std::pair<CObj<CWindow>, int> &p1, const std::pair<CObj<CWindow>, int> &p2 ) const
	{
		return p1.second > p2.second;
	}
};

typedef std::list<CStates> CStateSequiences;

class CWindowScreen : public CWindow, public IScreen
{
	OBJECT_NOCOPY_METHODS( CWindowScreen );

	CTooltips tooltips;

	CStateSequiences stateSequiences;
	CStateSequiences finishedAnimations; // finished reversable animations contained here

	// window may produce animation and command sequience. command sequence may
	// have run after animation is finished.
	CFreeIds animationIDs;

	typedef std::unordered_map<CWindowAnimationID, CStates, SAnimationIDHash> CWatingForAnimation;
	CWatingForAnimation watingForAnimation;

	// segment calling
	typedef std::unordered_set<CObj<IWindow>> CSegmentObjs;
	CSegmentObjs segmentObjs;

	// message reactions
	CMessageReactions messageReactions;
	// preprogrammed reactions
	CPtr<IProgrammedReactionsAndChecks> pReactionsAndChecks;
	CPtr<NDb::SWindowScreen> pInstance;

	CSortedVector<std::pair<CObj<CWindow>, int>, STabPairCompare> tabOrder;

	int nMouseButtonState;

	// Screen focus - flag that mean that screen(root window) is active. Another words: interface which contain this screen is top.
	bool bIsScreenFocused;
	bool bHudFontScale = false;

	NInput::CBind bindShift;
	NInput::CBind bindCtrl;
	NInput::CBind bindAlt;

protected:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

	uint16_t GetKeyboardFlags() const;

public:
	struct SButtonGroupID
	{
		int nGroupID;
		CPtr<IWindow> pParent;

		SButtonGroupID() {}

		SButtonGroupID( const int _nGroupID, IWindow *_pParent )
			: nGroupID( _nGroupID )
			, pParent( _pParent )
		{
		}

		bool operator==( const SButtonGroupID &id ) const
		{
			return id.nGroupID == nGroupID && id.pParent == pParent;
		}

		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &nGroupID );
			saver.Add( 2, &pParent );
			return 0;
		}
	};

	struct SButtonGroupIDHash
	{
		int operator()( const SButtonGroupID &id ) const
		{
			std::hash<const void *> pr;
			return ( pr( id.pParent ) << 4 ) & id.nGroupID;
		}
	};

private:
	typedef std::unordered_map<SButtonGroupID, CObj<CButtonGroup>, SButtonGroupIDHash> CButtonGroups;
	CButtonGroups buttonGroups;

	// all state descriptor sequiences possible on this screen
	// when some window generates command sequience it only
	// sends it's id
	typedef std::unordered_map<std::string, NDb::SUIStateSequence> CCommandSequiences;
	CCommandSequiences commandSequiences;

	static NGScene::I2DGameView *p2DGameView;
	CPtr<NGScene::IGameView> pInterface3DView;

	void InitScreen();
	void InitSingletonWindows();

	void ProcessStateSequiences( const int timeDiff );
	IUIEffector *RunStateCommand( const NDb::SUIStateBase &cmd );

	void RunStateCommandSequienceImmidiate( const std::string &szCmdSeq,
	                                        CWindow *pSequenceParent,
	                                        SWindowContext *pContext,
	                                        const bool bForward );
	bool ActivateNextInTabOrder(); // returns true if activated, otherwise returns false
public:
	CWindowScreen();

	void SetHudFontScale( bool bHud ) override
	{
		bHudFontScale = bHud;
	}

	bool UsesHudFontScale() const override
	{
		return bHudFontScale;
	}

	static NGScene::I2DGameView *Get2DGameView()
	{
		return p2DGameView;
	}

	void SetGView( NGScene::I2DGameView *_p2DGameView,
	               NGScene::IGameView *_pGameView,
	               NGScene::IGameView *_pInterface3DView ) override;

	NGScene::IGameView *GetInterface3DView() override
	{
		return pInterface3DView;
	}

	int operator&( struct IBinSaver &saver ) override;

	bool OnButtonDown( const CVec2 &vPos, const int nButton ) override;
	bool OnButtonUp( const CVec2 &vPos, const int nButton ) override;
	bool OnButtonDblClk( const CVec2 &vPos, const int nButton ) override;
	bool OnMouseMove( const CVec2 &vPos, const int nMouseState ) override;
	bool OnKey( const SGameMessage &msg );

	CButtonGroup *CreateButtonGroup( const int nID, IWindow *pButton, IWindow *pParent ) override;

	// run animation sequience on provided window. return ID of animation.
	int /*AnimationID*/ RunAnimationSequienceForward( const NDb::SUIStateSequence &seq, class CWindow *pWindow ) override;
	void RunAnimationSequienceBack( const int nAnimationID ) override;
	// run szCmdSeq effect. if nAnimationToWait provided, then wait untill animation is finished, then run szCmdSeq.
	void RunStateCommandSequience( const std::string &szCmdSeq,
	                               struct IWindow *pSequenceParent,
	                               SWindowContext *pContext,
	                               const bool bForward,
	                               const int nAnimationToWait = 0 ) override;
	// instantly undo command sequence (run in backward direction)
	void UndoStateCommandSequence( const std::string &szCmdSeq ) override;
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;
	void AfterLoad() override;

	bool RunReaction( const std::string &szSender, const std::string &szReactionName ) override;
	bool RunReaction( const std::string &szSender, const NDb::SUIDesc *pReaction ) override;
	void SetGView( NGScene::I2DGameView *_p2DGameView ) override;
	void Load( const struct NDb::SUIDesc *pDesc, IProgrammedReactionsAndChecks *pReactionsAndChecks ) override;
	void SetReactionsAndChecks( IProgrammedReactionsAndChecks *pReactionsAndChecks ) override;
	void Segment( const int timeDiff ) override;
	void UpdateResolution() override;
	void SetWindowText( const std::string &szWindowName, const std::wstring &szText ) override;
	void RegisterEffect( const std::string &szEffect, const NDb::SUIStateSequence &cmds ) override;
	void RegisterEffect( const std::string &szEffect,
	                     const std::vector<CDBPtr<NDb::SUIStateBase>> &cmds,
	                     const bool bReversable ) override;
	void RegisterReaction( const std::string &szReactionKey, struct IMessageReactionB2 *pReaction ) override;
	void RegisterToSegment( struct IWindow *pWnd, const bool bRegister ) override;
	bool IsRegisteredToSegment( struct IWindow *pWnd ) const override;
	void SetScreenSize( const CTRect<float> &rcScreen ) override;
	//virtual void OnMouseMove( const CVec2 &vPos, const int );
	IWindow *Pick( const CVec2 &vPos, const bool bRecursive ) override;
	void OnGetFocus( const bool bFocus ) override;
	void SetTooltipContext( const int nContext ) override;
	IWindow *CreateTooltipWindow( const std::wstring &wszTooltipText, IWindow *pTooltipOwner ) override;
	void Reposition( IWindow *pWindow = 0 ) override;
	bool ProcessEvent( const struct SGameMessage &msg ) override;
	void ProcessUIMessage( const struct SBUIMessage &msg ) override;

	IWindow *GetElement( const std::string &szName, const bool bRecursive = false ) override
	{
		return CWindow::GetChild( szName, bRecursive );
	}

	IWindow *GetVisibleElement( const std::string &szName, const bool bRecursive = false ) override
	{
		return CWindow::GetVisibleChild( szName, bRecursive );
	}

	IWindow *AddElement( const struct NDb::SUIDesc *pDesc ) override;
	void Enable( const bool bEnable ) override;

	static void Set2DGView( NGScene::I2DGameView *p2DGameView );

	const std::wstring &GetTextEntry( const std::string &szName ) const override;
	void RegisterTabOrder( IWindow *pWindow, int nTabOrder );
};
