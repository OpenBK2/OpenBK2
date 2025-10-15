// WindowMSButton.cpp: implementation of the CWindowMSButton class.
//


#include "stdafx.h"
#include "WindowMSButton.h"
#include "ButtonGroup.h"
#include "ForegroundTextString.h"
#include "System/Text.h"

REGISTER_SAVELOAD_CLASS(UI, 0x11075B87, CWindowMSButton)


// CWindowMSButton


void CButtonSubStateVisual::Init( const NDb::SButtonVisualSubState &substate )
{
	if ( substate.pBackground )
		pBackground = CUIFactory::MakeWindowPart( substate.pBackground );
	if ( substate.pForeground )
		pForeground = CUIFactory::MakeWindowPart( substate.pForeground );
	if ( substate.pTextString )
		pTextString = (CForegroundTextString*)( CUIFactory::MakeWindowPart( substate.pTextString ) );
	pTextFormat = substate.pTextFormat;
}

void CButtonSubStateVisual::SetupTextString( const NDb::SForegroundTextString *_pTextString )
{
	if ( !pTextString && _pTextString )
	{
		pTextString = (CForegroundTextString*)( CUIFactory::MakeWindowPart( _pTextString ) );
	}
}

void CWindowMSButton::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowMSButton *pDesc( checked_cast<const NDb::SWindowMSButton*>( _pDesc ) );
	pInstance = pDesc->Duplicate();

	CWindow::InitByDesc( _pDesc );

	pShared = checked_cast_ptr<const NDb::SWindowMSButtonShared *>( pDesc->pShared );

	NI_VERIFY( !pInstance->buttonStates.empty(), StrFmt( "ButtonStates not defined for button \"%s\"", pInstance->szName.c_str() ), return );
	NI_VERIFY( !pShared->visualStates.empty(), StrFmt( "VisualStates not defined for button \"%s\"", pInstance->szName.c_str() ), return );
	NI_VERIFY( pInstance->buttonStates.size() == pShared->visualStates.size(), StrFmt( "VisualStates is not equal ButtonStates for button \"%s\"", pInstance->szName.c_str() ), return );

	states.resize( pShared->visualStates.size() );
	for ( int i = 0; i < pShared->visualStates.size(); ++i )
	{
		states[i].Init( pShared->visualStates[i] );
//		states[i].SetupTextString( pInstance->pTextString );
	}
	nAnimationID = 0;
	bMouseEntered = false;
	bPressed.clear();
	bPressed.resize( 4, false );
	bEffect = false;
	eEffectSubState =  NDb::BST_NORMAL;

	if ( IsEnabled() )
		SwitchSubState( NDb::BST_NORMAL );
	else
		SwitchSubState( NDb::BST_DISABLED );

	/*for( int i = 0; i<states.size(); i++ )
	{
		SetState( i );
		wins.push_back( Singleton<IUIInitialization>()->CreateWindowFromDesc( this->GetDesc() ) );
	}
	for( int i = 0; i < wins.size(); i++ )
	{
		CTRect<float> rc( GetPlacement() );
		rc.MoveTo( 0, rc.Height()/3*i );
		wins[i]->SetPlacement( rc, EWPF_ALL );
		AddChild( wins[i], true );
	}*/
}

int CWindowMSButton::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CWindow*>( this ) );
	saver.Add( 2, &pInstance );
	saver.Add( 3, &pShared );
	saver.Add( 4, &bPressed );
	saver.Add( 5, &bMouseEntered );
	saver.Add( 7, &nAnimationID );
	saver.Add( 8, &states );
	saver.Add( 9, &pShared );
	saver.Add( 10, &bEffect );
	saver.Add( 11, &eEffectSubState );
	saver.Add( 12, &eOrigSubState );
	saver.Add( 13, &pButtonGroup );
	saver.Add( 14, &pButtonNotify );

	if ( saver.IsReading() )
	{
		nAnimationID = 0;
		bMouseEntered = false;
		bPressed.clear();
		bPressed.resize( 4, false );
	}
	return 0;
}

void CWindowMSButton::AfterLoad()
{
//	SetState( pInstance->nState );
	SwitchSubState( states[pInstance->nState].eSubState );
	CWindow::AfterLoad();
}

const std::wstring& CWindowMSButton::GetDBFormatText() const
{
	if ( const NDb::STextFormat *pTextFormat = pInstance->pTextFormat )
	{
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pTextFormat->, FormatString) )
			return GET_TEXT_PRE( pTextFormat->, FormatString );
	}
	
	return CWindow::GetDBFormatText();
}

const std::wstring& CWindowMSButton::GetDBInstanceText() const
{
	if ( CHECK_TEXT_NOT_EMPTY_PRE(pInstance->, Text) )
		return GET_TEXT_PRE(pInstance->, Text);

	return CWindow::GetDBInstanceText();
}

const NDb::SWindowPlacement* CWindowMSButton::GetDBTextPlacement() const
{
	if ( const NDb::STextFormat *pTextFormat = pInstance->pTextFormat )
	{
		return &pTextFormat->placement;
	}
	return CWindow::GetDBTextPlacement();
}

void CWindowMSButton::OnEnter( const int nButton )
{
	if ( bPressed[nButton] )
	{
		if ( !bMouseEntered )
		{
			BackAnimation();
			SwitchSubState( NDb::BST_PUSHED_DEEP );
			nAnimationID = GetScreen()->RunAnimationSequienceForward( pShared->visualStates[GetState()].pushed.onEnterSubState, this );
		}
	}
	else if ( 0 == nButton )
	{
		if ( !bMouseEntered )
		{
			BackAnimation();
			// switch to MOUSE_OVER static appearance
			SwitchSubState( NDb::BST_MOUSE_OVER );
			// run effect
			nAnimationID = GetScreen()->RunAnimationSequienceForward( pShared->visualStates[GetState()].mouseOver.onEnterSubState, this );
			if ( pButtonNotify )
				pButtonNotify->Entered( this );
		}
	}
	bMouseEntered = true;
}

void CWindowMSButton::OnLeave( const int nButton )
{
	if ( bMouseEntered )
	{
		// run ON_MOUSE_OVER in backward direction
		BackAnimation();
		SwitchSubState( NDb::BST_NORMAL );
		// run NORMAL in forward direction
		nAnimationID = GetScreen()->RunAnimationSequienceForward( pShared->visualStates[GetState()].normal.onEnterSubState, this );

		bMouseEntered = false;
		if ( pButtonNotify )
			pButtonNotify->Leaved( this );
	}
}

void CWindowMSButton::OnPush( const int nButton )
{
	if ( !bPressed[nButton] )
	{
		bPressed[nButton] = true;
		if ( nButton == MSTATE_BUTTON1 && CUIFactory::GetConsts() )
			GetScreen()->RunAnimationSequienceForward( CUIFactory::GetConsts()->buttonClickSound, this );
		// 
		if ( pShared->eTriggerMode == NDb::BCST_ON_PUSH )
		{
			// switch to next state
			// change state (static appearance )
			if ( nButton == MSTATE_BUTTON1 )
			{
				// run state NORMAL in backward direction
				BackAnimation();
				if ( pInstance->bAutoChangeState ) 
					SetNextState();
				// run logical state Sequience
				GetScreen()->RunAnimationSequienceForward( pInstance->buttonStates[GetState()].commandsOnEnterState, this );

				// acitvate pressed substate state in next state (static)
				SwitchSubState( NDb::BST_PUSHED_DEEP );
				// run pressed effect in forward direction
				nAnimationID = GetScreen()->RunAnimationSequienceForward( pShared->visualStates[GetState()].pushed.onEnterSubState, this );
				// run state change sequience and visual effects
				RunAnimationAndCommands( pShared->visualStates[GetState()].visualOnEnterState,
					pInstance->buttonStates[GetState()].szMessageOnEnterState,
					pInstance->buttonStates[GetState()].bWaitVisual, 
					pInstance->buttonStates[GetState()].bReverseCommands );
			}
			else if ( nButton == MSTATE_BUTTON2 )
			{
				// run state NORMAL in backward direction
				BackAnimation();

				// run logical state Sequience
				GetScreen()->RunAnimationSequienceForward( pInstance->buttonStates[GetState()].commandsOnRightClick, this );

				// acitvate pressed substate state in next state (static)
				SwitchSubState( NDb::BST_RIGHT_DOWN );
				// run RIGHT_DOWN effect in forward direction
				nAnimationID = GetScreen()->RunAnimationSequienceForward( pShared->visualStates[GetState()].rightButtonDown.onEnterSubState, this );
				// run state change sequience and visual effects
				RunAnimationAndCommands( pShared->visualStates[GetState()].visualOnEnterState,
					pInstance->buttonStates[GetState()].szMessageOnEnterState,
					pInstance->buttonStates[GetState()].bWaitVisual, 
					pInstance->buttonStates[GetState()].bReverseCommands );
			}
		}
		else
		{
			if ( nButton == MSTATE_BUTTON1 )
			{
				// run state NORMAL in backward direction
				BackAnimation();

				// acitvate pressed substate state in next state (static)
				SwitchSubState( NDb::BST_PUSHED_DEEP );
				// run PUSHED effect in forward direction
				nAnimationID = GetScreen()->RunAnimationSequienceForward( pShared->visualStates[GetState()].pushed.onEnterSubState, this );
			}
			else if ( nButton == MSTATE_BUTTON2 )
			{
				// run state NORMAL in backward direction
				BackAnimation();

				// acitvate pressed substate state in next state (static)
				SwitchSubState( NDb::BST_RIGHT_DOWN );
				// run RIGHT_DOWN effect in forward direction
				nAnimationID = GetScreen()->RunAnimationSequienceForward( pShared->visualStates[GetState()].rightButtonDown.onEnterSubState, this );
			}
		}

		// send notification only for left button !
		if ( pButtonNotify && nButton == MSTATE_BUTTON1 )
			pButtonNotify->Pushed( this );
	}
}

void CWindowMSButton::OnRelease( const bool bInside, const int nButton )
{
	if ( bPressed[nButton] )
	{
		if ( bInside )
		{
			bPressed[nButton] = false;

			if ( pShared->eTriggerMode == NDb::BCST_ON_RELEASE )
			{
				if ( nButton == MSTATE_BUTTON1 )
				{
					// run state PRESSED in backward direction
					BackAnimation();
					// change state (static appearance )
					if ( pInstance->bAutoChangeState ) 
						SetNextState();
					// run logical state Sequience
					NI_ASSERT( pInstance->buttonStates.size() > GetState(), StrFmt("button states(in instance) have less states then button states in shared.") );
					GetScreen()->RunAnimationSequienceForward( pInstance->buttonStates[GetState()].commandsOnEnterState, this );
					// run state change sequience and visual effects
					RunAnimationAndCommands( pShared->visualStates[GetState()].visualOnEnterState,
						pInstance->buttonStates[GetState()].szMessageOnEnterState,
						pInstance->buttonStates[GetState()].bWaitVisual, 
						!pInstance->buttonStates[GetState()].bReverseCommands );
				}
				else if ( nButton == MSTATE_BUTTON2 )
				{
					// run state PRESSED in backward direction
					BackAnimation();

					// run logical state Sequience
					NI_ASSERT( pInstance->buttonStates.size() > GetState(), StrFmt("button states(in instance) have less states then button states in shared.") );
					GetScreen()->RunAnimationSequienceForward( pInstance->buttonStates[GetState()].commandsOnRightClick, this );
					// run state change sequience and visual effects
					RunAnimationAndCommands( pShared->visualStates[GetState()].visualOnEnterState,
						pInstance->buttonStates[GetState()].szMessageOnEnterState,
						pInstance->buttonStates[GetState()].bWaitVisual, 
						!pInstance->buttonStates[GetState()].bReverseCommands );
				}
			}
			// change substate ( static appearance )
			if ( eOrigSubState != NDb::BST_DISABLED )
				SwitchSubState( NDb::BST_MOUSE_OVER );

			// run state MOUSE_OVER in forward direction
			nAnimationID = GetScreen()->RunAnimationSequienceForward( pShared->visualStates[GetState()].mouseOver.onEnterSubState, this );
		}
		else
		{
			BackAnimation();

			// set state to normal ( just static appearance )
			SwitchSubState( NDb::BST_NORMAL );
			nAnimationID = RunAnimationAndCommands( pShared->visualStates[GetState()].normal.onEnterSubState, "", false, true );
		}
		if ( pButtonNotify && nButton == MSTATE_BUTTON1 )
			pButtonNotify->Released( this );
	}
}

void CWindowMSButton::OnRightClick()
{
	
}

void CWindowMSButton::BackAnimation()
{
	GetScreen()->RunAnimationSequienceBack( nAnimationID );
	nAnimationID = 0;
}

void CWindowMSButton::SetNextState()
{
	if ( !pButtonGroup || pButtonGroup->TrySwitchState( this ) )
	{
		++pInstance->nState;
		pInstance->nState %= states.size();
		SwitchSubState( NDb::BST_NORMAL );

		if ( pButtonNotify )
			pButtonNotify->StateChanged( this );
	}
}

void CWindowMSButton::NotifyStateSequenceFinished()
{
}

void CWindowMSButton::OnActivatePushedState( const SGameMessage &msg )
{
	if ( msg.nParam1 )
		SwitchSubState( NDb::BST_PUSHED_DEEP );
	else
		SwitchSubState( NDb::BST_NORMAL );
}

void CWindowMSButton::Init()
{
	CWindow::Init();
	
	//bEnabled and substate must by synchronized:
	//SwitchSubState( NDb::BST_NORMAL );
	Enable( IsEnabled() );

	bMouseEntered = false;
	if ( pInstance->nButtonGroupID )
	{
		SetButtonGroup( pInstance->nButtonGroupID );
	}
}

CButtonGroup* CWindowMSButton::GetButtonGroup()
{
	return pButtonGroup;
}

void CWindowMSButton::SetButtonGroup( const int nGroup )
{
	pButtonGroup = GetScreen()->CreateButtonGroup( pInstance->nButtonGroupID, this, GetParent() );
	pButtonGroup->Add( this );
}

bool CWindowMSButton::OnButtonDown( const CVec2 &vPos, const int nButton )
{
	if ( CWindow::OnButtonDown( vPos, nButton ) )
		return true;
	if ( !IsInside( vPos ) ) 
		return false;
	if ( !IsEnabled() )
		return true;
	OnPush( nButton );
	return true;
}

bool CWindowMSButton::OnButtonUp( const CVec2 &vPos, const int nButton )
{
	if ( CWindow::OnButtonUp( vPos, nButton ) )
		return true;

	if ( !bPressed[nButton] || !IsEnabled() ) 
		return false;

	const bool bInside = IsInside( vPos );
	if ( nButton == MSTATE_BUTTON1 )
	{
		if ( states[pInstance->nState].eSubState == NDb::BST_PUSHED_DEEP )
			OnRelease( bInside, nButton );
		bPressed[nButton] = false;
		OnMouseMove( vPos, nButton & ~MSTATE_BUTTON1 );
	}
	else if ( nButton == MSTATE_BUTTON2 )
	{
		if ( states[pInstance->nState].eSubState == NDb::BST_RIGHT_DOWN )
			OnRelease( bInside, nButton );
		bPressed[nButton] = false;
		OnMouseMove( vPos, nButton & ~MSTATE_BUTTON2 );
	}
	return bInside;
}

bool CWindowMSButton::OnButtonDblClk( const CVec2 &_vPos, const int nButton )
{
	if ( !CWindow::OnButtonDblClk( _vPos, nButton ) )
	{
		NI_ASSERT( pInstance->buttonStates.size() > GetState(), StrFmt("button states(in instance) have less states then button states in shared.") );
		if ( !pShared->bIgnoreDblClick && (nButton & MSTATE_BUTTON1) && !pInstance->buttonStates[GetState()].commandsOnLDblKlick.commands.empty() )
		{
			GetScreen()->RunAnimationSequienceForward( pInstance->buttonStates[GetState()].commandsOnLDblKlick, this );
			return true;
		}
	}
	return false;
}

bool CWindowMSButton::OnMouseMove( const CVec2 &vPos, const int nButton )
{
	if ( IsEnabled() )
	{
		if ( bPressed[0] && !(nButton & MSTATE_BUTTON1) )
			OnRelease( false, MSTATE_BUTTON1 );
		if ( bPressed[1] && !(nButton & MSTATE_BUTTON2) )
			OnRelease( false, MSTATE_BUTTON2 );

		const bool bInside = IsInside( vPos );
		if ( bInside )
			OnEnter( nButton );
		else
			OnLeave( nButton );
	}
	return false;
}

void CWindowMSButton::Enable( const bool bEnable )
{
	if ( bEnable )
	{
		if ( states.empty() )
			return;

		if ( states[GetState()].eSubState == NDb::BST_DISABLED )
			SwitchSubState( NDb::BST_NORMAL );
	}
	else
		SwitchSubState( NDb::BST_DISABLED );
	CWindow::Enable( bEnable );
}

void CWindowMSButton::SetStateWithVisual( const int nState )
{
	BackAnimation();
	SetState( nState );
	// run state change sequience and visual effects
	RunAnimationAndCommands( pShared->visualStates[GetState()].visualOnEnterState,
		pInstance->buttonStates[GetState()].szMessageOnEnterState,
		pInstance->buttonStates[GetState()].bWaitVisual, 
		!pInstance->buttonStates[GetState()].bReverseCommands );

	if ( !IsEnabled() )
		Enable( false );
	else // change substate ( static appearance )
		SwitchSubState( NDb::BST_NORMAL );
}

void CWindowMSButton::SetState( const int nState )
{
	pInstance->nState = nState % states.size();
	SwitchSubState( NDb::BST_NORMAL );
}

void CWindowMSButton::SetTextString( const std::wstring &wszText )
{
	wszCustomText = wszText;
/*	for ( int i = 0; i < states.size(); ++i )
	{
		if ( states[i].normal.pTextString )
			states[i].normal.pTextString->SetText( szText );

		if ( states[i].mouseOver.pTextString )
			states[i].mouseOver.pTextString->SetText( szText );

		if ( states[i].pushed.pTextString )
			states[i].pushed.pTextString->SetText( szText );

		if ( states[i].disabled.pTextString )
			states[i].disabled.pTextString->SetText( szText );
	}*/
	CWindow::SetTextString( wszText );
}

void CWindowMSButton::SetOutline( const CDBID &outlineType )
{
	CWindow::SetOutline( outlineType );	
	SwitchSubStatePrivate();
}

void CWindowMSButton::SwitchSubState( const NDb::EButtonSubstateType _eSubState )
{
	eOrigSubState = _eSubState;
	SwitchSubStatePrivate();
}

void CWindowMSButton::SwitchSubStatePrivate()
{
	NDb::EButtonSubstateType eSubState = bEffect ? eEffectSubState : eOrigSubState;
	CButtonSubStateVisual &substate = states[pInstance->nState].Substates( eSubState );
	//if ( states[static_cast<NDb::SWindowMultiBkg*>( GetInstance() )->nState].substates[substate].pBackground )
	SetBackground( substate.pBackground );

	//if ( states[pInstance->nState].substates[substate].pForeground )
	SetForeground( substate.pForeground );
	//if ( states[pInstance->nState].substates[substate].pTextString )
	// apply substate's text/format
	ApplySubstateText( substate );
	
	states[pInstance->nState].eSubState = eSubState;
}

void CWindowMSButton::SetEffectSubState( const NDb::EButtonSubstateType _eSubState, bool _bEffect )
{
	eEffectSubState = _eSubState;
	bEffect = _bEffect;
	SwitchSubStatePrivate();
}

int CWindowMSButton::GetState( const std::string &szName )
{
	int nCount = pInstance->buttonStates.size();
	for ( int i = 0; i < nCount; ++i )
	{
		if ( pInstance->buttonStates[i].szName == szName )
			return i;
	}
	return -1;
}

void CWindowMSButton::SetTexture( const struct NDb::STexture *pDesc )
{
	for ( int i = 0; i < states.size(); ++i )
	{
		if ( states[i].normal.pBackground )
			states[i].normal.pBackground->SetTexture( pDesc );

		if ( states[i].mouseOver.pBackground )
			states[i].mouseOver.pBackground->SetTexture( pDesc );

		if ( states[i].pushed.pBackground )
			states[i].pushed.pBackground->SetTexture( pDesc );

		if ( states[i].disabled.pBackground )
			states[i].disabled.pBackground->SetTexture( pDesc );

		if ( states[i].rightButtonDown.pBackground )
			states[i].rightButtonDown.pBackground->SetTexture( pDesc );
	}
}

void CWindowMSButton::ApplySubstateText( const CButtonSubStateVisual &substate )
{
	std::wstring wszText;
	if ( !wszCustomText.empty() )
	{
		wszText = wszCustomText;
	}
	else
	{
		wszText = GetDBInstanceText();
		if ( substate.pTextFormat && CHECK_TEXT_NOT_EMPTY_PRE(substate.pTextFormat->,FormatString) )
		{
			wszText = GET_TEXT_PRE(substate.pTextFormat->,FormatString) + wszText;
		}
		else if ( substate.pTextString )
		{
			wszText = substate.pTextString->GetDBFormatText() + wszText;
		}
		else
		{
			wszText = GetDBFormatText() + wszText;
		}
	}
	
	if ( substate.pTextFormat )
	{
		SetTextPlacement( substate.pTextFormat->placement );
	}
	else if ( substate.pTextString )
	{
		if ( const NDb::SWindowPlacement *pPlacement = substate.pTextString->GetPlacement() )	
			SetTextPlacement( *pPlacement );
	}
	else
	{
		if ( const NDb::SWindowPlacement *pPlacement = GetDBTextPlacement() )
			SetTextPlacement( *pPlacement );
	}	

	CWindow::SetTextString( wszText );
}

void CWindowMSButton::Reposition( const CTRect<float> &parentRect )
{
	CWindow::Reposition( parentRect );
}

void CWindowMSButton::Visit( struct IUIVisitor *pVisitor )
{
	CWindow::Visit( pVisitor );
}


