#pragma once
// Window.h: interface for the CWindow class.
//

#include "Misc/HashFuncs.h"
#include "Misc/Heap.h"
#include "UI/UIFactory.h"
#include "WindowMessageHandle.h"

#include "UI_export.h"

#include <cstdint>

class CForegroundTextString;
class CWindow;

struct SWindowCompare
{
	bool operator()( const CObj<CWindow> &o1, const CObj<CWindow> &o2 ) const;
};

typedef std::pair<int /*animation ID*/, CPtr<CWindow>> CWindowAnimationID;

struct SAnimationIDHash
{
	int operator()( const CWindowAnimationID &p ) const;
};

enum EMouseStateB2
{
	MSTATE_FREE = 0,
	MSTATE_BUTTON1 = 1,
	MSTATE_BUTTON2 = 2,
};

class UI_EXPORT CUIMORegConttainer : public NInput::CGMORegContainer
{
public:
	virtual ~CUIMORegConttainer() = default;
	void AddRawObserver( const std::string &szMsgName, IGMObserver *pObserver ) override;
};

const int WINDOW_TOP_PRIORITY = 0x01000000;

// base class to all UI windows;
// single background window.
class UI_EXPORT CWindow : virtual public IWindow, public CUIMORegConttainer
{
	//OBJECT_BASIC_METHODS(CWindow)
	bool bIsModal;
	CDBPtr<NDb::SWindow> pWindowStats;
	CObj<CForegroundTextString> pTextString;

	// dynamic data, set during execution
	CPtr<IWindow> pCustomTooltip;
	std::wstring wszCustomTooltip;

	CPtr<CWindow> pParent;       // parent window.
	CObj<CWindow> pFocusedChild; // child or its parent that has keyboard focus
	bool bFocused;
	CDBID nOutlineType;
	CObj<CWindow> pHighlighted;         // window currently under mouse cursor
	std::vector<CObj<CWindow>> pressed; // pressed with each mouse button
	CVec2 vScreenPos;
	IClickNotify *pClickNotify;

	CObj<class CPlacedText> pPlacedText;

	bool bDelayChildRemove; // valid only during ProcessEvent
	std::vector<CObj<CWindow>> removedChildren;
	float fFadeValue;
	float fInternalFadeValue;
	int nIDForMLHandler;

protected:
	CObj<IWindowPart> pBackground; // may be 0
	CObj<IWindowPart> pForeground;
	typedef CSortedVector<CObj<CWindow>, SWindowCompare> CDrawOrder;
	CDrawOrder drawOrder;

	CUIMORegConttainer priorityEvents; // events for that parent have priority over children
	CPtr<SWindowContext> pContext;

private:
	// end dynamic data

	// BEGIN these loads from data

	typedef std::unordered_set<CPtr<CWindow>> CChildren;
	CChildren children;
	std::pair<CTRect<float>, bool> delayedReposition;

	// message handler
	DECLARE_HANDLE_MAP;
	DECLARE_MESSAGE_HANDLER( ShowWindow );
	DECLARE_MESSAGE_HANDLER( SetFocus );
	DECLARE_MESSAGE_HANDLER( Enable );

	void ProcessFocused( CWindow *pChild );
	void ProcessUnfocused( CWindow *pChild );
	void DelayedChildRemove();

protected:
	CDBPtr<NDb::SWindowShared> pShared;
	virtual NDb::SWindow *GetInstance() = 0;

	//g trick not to create 2 virtual functions.
	const NDb::SWindow *GetInstance() const
	{
		return const_cast<CWindow *>( this )->GetInstance();
	}

	CWindow();

	void SetTextPlacement( const struct NDb::SWindowPlacement &placement );

	CWindow *PickInternal( const CVec2 &vPos, const bool bRecursive );

	const char *GetPressedName( const int nButton )
	{
		if ( pressed.size() > nButton && pressed[nButton] )
		{
			return pressed[nButton]->GetName().c_str();
		}
		return 0;
	}

	int RunAnimationAndCommands( const NDb::SUIStateSequence &seq,
	                             const std::string &szCommanEffect,
	                             const bool bWaitOnGraphics,
	                             const bool bForward );

	// Returns root window
	CWindow *GetRoot();

	// Returns a final focused window (not any of its parents)
	CWindow *FindFocusedWindow();
	// Removes focus from a focused window
	void RemoveAnyFocus();
	// if pressed window is not focused window (or its parent) - remove focus from former window
	void CheckRemoveFocus( CWindow *pClickedWnd );

	// for debug purpose only
	int CheckNamedChildrenCount( const std::string &szName, bool bRecursive );

	virtual const std::wstring &GetDBFormatText() const;
	virtual const std::wstring &GetDBInstanceText() const;
	virtual const NDb::SWindowPlacement *GetDBTextPlacement() const;
	virtual void OnChangeVisibility( bool bShow );

	virtual bool IsRelatedFocus( IWindow *pWindow ) const
	{
		return this == pWindow;
	}

	float GetTotalFadeValue() const
	{
		return fFadeValue * fInternalFadeValue;
	}

	virtual bool IsActiveArea( const CVec2 &vPos ) const;
	void VisitText( struct IUIVisitor *pVisitor );

public:
	void InitStatic();

	//{ access to flags
	bool IsTransparent() const
	{
		return pShared->flags.bTransparent;
	}

	virtual bool IsPickable() const;
	//}

	int GetPriority() const override;
	void
	SetPriority( int n ) override; // works only before insertion into parent due to there is no children resort provided

	// serializing...
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;

	int operator&( IBinSaver &ss ) override;

	// fills rect with on-screen coordinates
	void FillWindowRect( CTRect<float> *pRect ) const override;
	CTRect<float> GetWindowRect() const override;
	// if child window provieed, then reposition only that window.
	// otherwize reposition all child windows
	void RepositionChildren( IWindow *pWindow = 0 );
	virtual void Reposition( const CTRect<float> &parentRect );
	void Init() override;

	// window may want to be notified about finish state sequience, that it launched
	virtual void NotifyStateSequenceFinished() {}

	// placement flags = OR of number EWindowPlacementFlags
	void GetPlacement( int *pX, int *pY, int *pSizeX, int *pSizeY ) const override;
	void
	SetPlacement( const float x, const float y, const float sizeX, const float sizeY, const uint32_t flags ) override;
	CTRect<int> GetPlacement() const override;
	void SetPlacement( const CTRect<int> &rc, const uint32_t flags ) override;

	struct IScreen *GetScreen() override;
	bool UsesHudFontScale() const override;
	// is point (in screen coordinates) inside control
	bool IsInside( const CVec2 &vPos ) const override;
	// Gives or removes focus from the current window
	void SetFocus( const bool bFocus ) override;

	bool IsFocused() const override
	{
		return bFocused;
	}

	void ShowWindow( const bool bShow ) override;
	bool IsVisible() const override;

	const NDb::EPositionAllign GetVerAllign() const
	{
		return GetInstance()->placement.verAllign.first;
	}

	const NDb::EPositionAllign GetHorAllign() const
	{
		return GetInstance()->placement.horAllign.first;
	}

	void SetAllign( const NDb::EPositionAllign eHorAllign, const NDb::EPositionAllign eVerAllign )
	{
		GetInstance()->placement.horAllign = eHorAllign;
		GetInstance()->placement.verAllign = eVerAllign;
	}

	void SetClickNotify( IClickNotify *pNotify )
	{
		pClickNotify = pNotify;
	}

	// children/parent work
	void AddChild( IWindow *pWnd, bool bDoReposition ) override;
	// close window (remove it from parent's children stack)
	void Close();
	// immidiate window children
	int GetNumChildren() override;
	IWindow *GetChild( int nIndex ) override;
	void RemoveChild( IWindow *_pChild ) override;
	void RemoveChild( const std::string &szChildName );
	IWindow *GetChild( const std::string &_szName, const bool bRecursive ) override;
	IWindow *GetVisibleChild( const std::string &_szName, const bool bRecursive ) override;
	// deep children
	CWindow *GetDeepChild( const std::string &_szName );
	virtual void SetParent( CWindow *pParent );

	CWindow *GetParent() const
	{
		return pParent;
	}

	const std::string &GetName() const override;
	void SetName( const std::string &_szName ) override;
	// return minimal width that text is displayed properly
	int GetOptimalWidth() const;

	// broadcast UI message processing
	// return true if message is processed and don't need to process it anymore
	// generally, if Screen returns false, that means something wriong with this message
	// (no message sing exists on the screen - wrong situation or wrong message ID)
	bool ProcessEvent( const struct SGameMessage &msg ) override;
	bool ProcessMessage( const struct SBUIMessage &msg ) override;

	void SetModal( bool _bIsModal ) override
	{
		bIsModal = _bIsModal;
	}

	bool IsModal() const override
	{
		return bIsModal;
	}

	//translators of "msg" into "vPos" and "nButton"
	bool MsgOnLMouseDown( const struct SGameMessage &msg );
	bool MsgOnLMouseUp( const struct SGameMessage &msg );
	bool MsgOnLMouseDblClick( const struct SGameMessage &msg );
	bool MsgOnRMouseDown( const struct SGameMessage &msg );
	bool MsgOnRMouseUp( const struct SGameMessage &msg );
	bool MsgOnRMouseDblClick( const struct SGameMessage &msg );
	bool MsgOnMouseMove( const struct SGameMessage &msg );
	//void MsgOnChar( const struct SGameMessage &msg );
	// IWindow implementation
	bool OnButtonDown( const CVec2 &vPos, const int nButton ) override;
	bool OnButtonUp( const CVec2 &vPos, const int nButton ) override;
	bool OnButtonDblClk( const CVec2 &vPos, const int nButton ) override;
	bool OnMouseMove( const CVec2 &vPos, const int nMouseState ) override;
	//virtual void OnChar( const wchar_t chr );
	IWindow *Pick( const CVec2 &vPos, const bool bRecursive ) override;
	void SetOutline( const CDBID &outlineType = CDBID() ) override;
	void SetTextString( const std::wstring &szText ) override;
	const wchar_t *GetTextString() const override;
	std::wstring GetDBText() const override;
	SWindowContext *GetContext() override;

	const struct NDb::SUIDesc *GetDesc() const override
	{
		return pWindowStats;
	}

	IWindow *GetParentWindow() const override
	{
		return pParent;
	}

	const struct NDb::SWindowShared *GetSharedDesc() const override
	{
		return pShared;
	}

	IWindow *GetChild( const int _nTypeID, const int _nID, const bool bRecursive = false ) override;
	bool IsPointInsideOfChildren( const CVec2 &vPoint ) override;

	// game message sink
	void GameMessageSink( const struct SGameMessage &msg, int nMessageType );

	//get manipulator for editor functionality
	// help context
	const std::wstring &GetDBTooltipStr() const override; // old function, need to be removed
	IWindow *DemandTooltip() override;
	//
	void SetTooltip( const std::wstring &wszTooltip ) override;
	void SetTooltipIDForMLHandler( int nID ) override;
	int GetTooltipIDForMLHandler() const override;
	void SetTooltip( IWindow *pTooltipWindow ) override;

	// DRAWING
	void Visit( struct IUIVisitor *pVisitor ) override;
	void RegisterObservers() override;
	// friends
	friend struct SWindowCompare;
	friend class CUIMessageHandler;

	//enabled / disabled
	void Enable( const bool bEnable ) override;

	bool IsEnabled() const override
	{
		return GetInstance()->bEnabled;
	}

	void SetBackground( IWindowPart *_pBackground ) override;
	void SetForeground( IWindowPart *_pForeground );
	void CopyBackground( const IWindow *pSrcWnd ) override;
	void SetTexture( const struct NDb::STexture *pDesc ) override;
	void AfterLoad() override;
	void SetFadeValue( float fValue ) override;
	void SetInternalFadeValue( float fValue );
};
