// WindowMSButton.h: interface for the CWindowMSButton class.
//


#if !defined(AFX_WINDOWMSBUTTON_H__CBE5FDBF_1AE1_48E8_8464_1E6952CB132E__INCLUDED_)
#define AFX_WINDOWMSBUTTON_H__CBE5FDBF_1AE1_48E8_8464_1E6952CB132E__INCLUDED_

#pragma ONCE

#include "Window.h"


class CButtonSubStateVisual
{	
public:
	CObj<IWindowPart> pBackground;									// background for this substats
	CObj<IWindowPart> pForeground;									// drawed after all window contents
	CObj<CForegroundTextString> pTextString;				// text string
	CDBPtr<NDb::STextFormat> pTextFormat;
	
	CButtonSubStateVisual() {  }
	
	void Init( const NDb::SButtonVisualSubState &substate );
	void SetupTextString( const NDb::SForegroundTextString *pTextString );

	bool IsValid() const 
	{ 
		return pBackground || pForeground || pTextString;
	}

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &pBackground );
		saver.Add( 2, &pForeground );
		saver.Add( 3, &pTextString );
		saver.Add( 4, &pTextFormat );
		return 0;
	}
};

class CButtonState
{
public:
	CButtonSubStateVisual normal;
	CButtonSubStateVisual mouseOver;
	CButtonSubStateVisual pushed;
	CButtonSubStateVisual disabled;
	CButtonSubStateVisual rightButtonDown;
	NDb::EButtonSubstateType eSubState;

	CButtonState() : eSubState( NDb::BST_NORMAL ) { }
	void Init( const NDb::SButtonVisualState &state )
	{
		normal.Init( state.normal );
		mouseOver.Init( state.mouseOver );
		if ( !mouseOver.IsValid() )
			mouseOver.Init( state.normal );
		pushed.Init( state.pushed );
		if ( !pushed.IsValid() )
			pushed.Init( state.normal );
		disabled.Init( state.disabled );
		if ( !disabled.IsValid() )
			disabled.Init( state.normal );
		rightButtonDown.Init( state.rightButtonDown );
		if ( !rightButtonDown.IsValid() )
			rightButtonDown.Init( state.normal );
	}
	void SetupTextString( const NDb::SForegroundTextString *pTextString )
	{
		if ( pTextString )
		{
			normal.SetupTextString( pTextString );
			mouseOver.SetupTextString( pTextString );
			pushed.SetupTextString( pTextString );
			disabled.SetupTextString( pTextString );
			rightButtonDown.SetupTextString( pTextString );
		}
	}

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &normal );
		saver.Add( 2, &mouseOver );
		saver.Add( 3, &pushed );
		saver.Add( 4, &disabled );
		saver.Add( 5, &eSubState );
		saver.Add( 6, &rightButtonDown );
		return 0;
	}
	CButtonSubStateVisual & Substates( const NDb::EButtonSubstateType _eSubState )
	{
		switch( _eSubState )
		{
		case NDb::BST_NORMAL:
			return normal;
		case NDb::BST_MOUSE_OVER:
			return mouseOver;
		case NDb::BST_PUSHED_DEEP:
			return pushed;
		case NDb::BST_DISABLED:
			return disabled;
		case NDb::BST_RIGHT_DOWN:
			return rightButtonDown;
		default:
			return normal;
		}
	}
	CButtonSubStateVisual & Substates( const int _eSubState )
	{
		return Substates( static_cast<const NDb::EButtonSubstateType>( _eSubState ) );
	}
};
class CButtonGroup;

// Multi State Button may have any number of states (must be equal in number to graphical states )
// generate Effector sequiences for 
// MouseEnter/MouseLeave
// Pressed/Released
// Also button is registered for game messages.
class CWindowMSButton : public CWindow, public IButton
{
	OBJECT_BASIC_METHODS( CWindowMSButton );

	CPtr<NDb::SWindowMSButton> pInstance;
	CDBPtr<NDb::SWindowMSButtonShared> pShared;
	
	vector<CButtonState> states;
	//vector<CObj<IWindow> > wins;
	// dynamic data
	//IButtonNotify * pButtonNotify;
	CPtr<IButtonNotify> pButtonNotify;
	vector<int> bPressed;
	bool bMouseEntered;
	int nAnimationID;
	// end dynamic data
	// group that this button in
	CObj<CButtonGroup> pButtonGroup;
	//
	bool bEffect;
	NDb::EButtonSubstateType eEffectSubState;
	NDb::EButtonSubstateType eOrigSubState;

//	CObj<class CPlacedText> pPlacedText;
	wstring wszCustomText;
private:
	void OnEnter( const int nButton );
	void OnLeave( const int nButton );

	void OnPush( const int nButton );
	void OnRelease( const bool bInside, const int nButton );

	void OnRightClick();

	int GetNStates() const { return states.size(); }
	void SwitchSubState( const NDb::EButtonSubstateType eSubState );
	void SwitchSubStatePrivate();

	void BackAnimation();
	
	void ApplySubstateText( const CButtonSubStateVisual &substate );
protected:
	NDb::SWindow* GetInstance()
	{
		return pInstance;
	}

	//{ overrided
	const wstring& GetDBFormatText() const;
	const wstring& GetDBInstanceText() const;
	const NDb::SWindowPlacement* GetDBTextPlacement() const;
	//}
public:
	void SetTextString( const wstring &wszText );
	void SetOutline( const CDBID &outlineType );
	void Visit( interface IUIVisitor *pVisitor );

	void OnActivatePushedState( const SGameMessage &msg );

	CWindowMSButton() : bPressed( 4, false ), pButtonNotify( 0 ), bMouseEntered( false ), nAnimationID( 0 ) {  }
	
	bool OnMouseMove( const CVec2 &vPos, const int nButton );
	bool OnButtonDown( const CVec2 &vPos, const int nButton );
	bool OnButtonUp( const CVec2 &vPos, const int nButton );
	bool OnButtonDblClk( const CVec2 &vPos, const int nButton );

	void SetNotifySink( interface IButtonNotify *pNotify ) { pButtonNotify = pNotify; }

	int operator&( interface IBinSaver &saver );
	void InitByDesc( const struct NDb::SUIDesc *pDesc );

	void NotifyStateSequenceFinished();
	void Init();
	bool IsPushed() const { return states[pInstance->nState].eSubState == NDb::BST_PUSHED_DEEP; }

	// void Enable( const bool bEnable );

	void SetNextState();
	void SetState( const int nState );
	void SetStateWithVisual( const int nState );
	int GetState() const { return pInstance->nState;	}
	class CButtonGroup* GetButtonGroup();
	void SetButtonGroup( const int nGroup );
	void SetEffectSubState( const NDb::EButtonSubstateType eSubState, bool bEffect );
	int GetState( const string &szName );
	void SetTexture( const struct NDb::STexture *pDesc );

	//enabled / disabled
	void Enable( const bool bEnable );
	// bool IsEnabled() const = 0;
	void AfterLoad();

	void Reposition( const CTRect<float> &parentRect );
};

#endif // !defined(AFX_WINDOWMSBUTTON_H__CBE5FDBF_1AE1_48E8_8464_1E6952CB132E__INCLUDED_)

