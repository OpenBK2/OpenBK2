// UIScreen.cpp: implementation of the CWindowScreen class.
//


#include "stdafx.h"
#include "UIScreen.h"

//#include "Cursor.h"
#include "ButtonGroup.h"
#include "../3Dmotor/G2DView.h"
#include "../System/Text.h"


REGISTER_SAVELOAD_CLASS(0x11075B80,CWindowScreen)


// CWindowScreen


NGScene::I2DGameView *CWindowScreen::p2DGameView = 0;

void StartEffectAndDeleteIfNeeded( CStateSequiences::iterator *ss, CStateSequiences *src, CStateSequiences *dst, CWindowScreen *pS )
{
	(*ss)->Segment( 1, pS ); // advance a little (to run all imidiate reactions)
	if ( (*(*ss)).IsEnd() )
	{
		if ( (*(*ss)).IsToBeDeleted() || !dst )
			*ss = (*src).erase( *ss );
		else																// animation finished, but may be reversed.
			(*dst).splice( (*dst).end(), (*src), (*ss)++ );
	}
}

CWindowScreen::CWindowScreen()
: nMouseButtonState( 0 ),
	bindShift( "shift_key" ),
	bindCtrl( "ctrl_key" ),
	bindAlt( "alt_key" ),
	bIsScreenFocused( false )
{	
	priorityEvents.AddObserver( "win_left_button_down", &CWindowScreen::MsgOnLMouseDown );
	priorityEvents.AddObserver( "win_left_button_up", &CWindowScreen::MsgOnLMouseUp );
	priorityEvents.AddObserver( "win_left_button_dblclk", &CWindowScreen::MsgOnLMouseDblClick );
	priorityEvents.AddObserver( "win_right_button_down", &CWindowScreen::MsgOnRMouseDown );
	priorityEvents.AddObserver( "win_right_button_up", &CWindowScreen::MsgOnRMouseUp );
	priorityEvents.AddObserver( "win_right_button_dblclk", &CWindowScreen::MsgOnRMouseDblClick );
	priorityEvents.AddObserver( "win_mouse_move", &CWindowScreen::MsgOnMouseMove );
	AddObserver( "win_key", &CWindowScreen::OnKey );
}

void CWindowScreen::UpdateResolution()
{
	const CVec2 vpSize = p2DGameView->GetViewportSize();
	Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( vpSize.x, vpSize.y );
	p2DGameView->SetWindowSize( vpSize );
	if ( GetInstance() )
		Reposition();
}

void CWindowScreen::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowScreen *pDesc( checked_cast<const NDb::SWindowScreen*>( _pDesc ) );
	pInstance = pDesc->Duplicate();

	CWindow::InitByDesc( _pDesc );
	messageReactions.InitByDesc( pDesc->messageReactions );
	
	CWindow::InitStatic();
	
	for ( vector<NDb::SCommandSequienceEntry>::const_iterator it = pDesc->commandSequiences.begin();
				it != pDesc->commandSequiences.end(); ++it )
	{
		NI_ASSERT( commandSequiences.find( it->szName ) == commandSequiences.end(), StrFmt( "duplicate reaction name \"%s\"", it->szName.c_str() ) );
		commandSequiences[it->szName] = it->sequence;
	}
}

bool CWindowScreen::ProcessEvent( const struct SGameMessage &msg )
{
	bindShift.ProcessEvent( msg );
	bindCtrl.ProcessEvent( msg );
	bindAlt.ProcessEvent( msg );
	if ( msg.mMessage.cType == NInput::CT_UNKNOWN && !CUIFactory::IsMessageRegistered( msg ) )
		return false;
	
	return CWindow::ProcessEvent( msg );
}

void CWindowScreen::ProcessUIMessage( const struct SBUIMessage &msg )
{
	CWindow::ProcessMessage( msg );
}

bool CWindowScreen::OnButtonDown( const CVec2 &vPos, const int nButton )
{
	nMouseButtonState |= nButton;
	CVec2 vPosOnScreen;
	ScreenToVirtual( vPos, &vPosOnScreen );
	return CWindow::OnButtonDown( vPosOnScreen, nButton );
}

bool CWindowScreen::OnButtonUp( const CVec2 &vPos, const int nButton )
{
	nMouseButtonState &= ~nButton;
	CVec2 vPosOnScreen;
	ScreenToVirtual( vPos, &vPosOnScreen );
	return CWindow::OnButtonUp( vPosOnScreen, nButton );
}

bool CWindowScreen::OnButtonDblClk( const CVec2 &vPos, const int nButton )
{
	CVec2 vPosOnScreen;
	ScreenToVirtual( vPos, &vPosOnScreen );
	return CWindow::OnButtonDblClk( vPosOnScreen, nButton );
}

bool CWindowScreen::OnMouseMove( const CVec2 &vPos, const int nMouseState )
{
	CVec2 vPosOnScreen;
	ScreenToVirtual( vPos, &vPosOnScreen );
	const bool bRet = CWindow::OnMouseMove( vPosOnScreen, nMouseButtonState );
	tooltips.OnMouseMove( vPos, nMouseState, this );
	return bRet;
}

bool CWindowScreen::OnKey( const SGameMessage &msg )
{
	if ( msg.nParam1 == VK_TAB )
	{
		if ( tabOrder.Size() == 0 )
			return false;
		else
			return ActivateNextInTabOrder();
	}
	return false;
}

void CWindowScreen::Enable( const bool bEnable )
{
	nMouseButtonState = 0;
	CWindow::Enable( bEnable );
}

void CWindowScreen::Reposition( IWindow *pWindow )
{
	RepositionChildren( pWindow );
}

IWindow *CWindowScreen::Pick( const CVec2 &vPos, const bool bRecursive )
{
	CVec2 vPosOnScreen;
	ScreenToVirtual( vPos, &vPosOnScreen );
	return PickInternal( vPosOnScreen, bRecursive );
}

CButtonGroup * CWindowScreen::CreateButtonGroup( const int nID, IWindow *pButton, IWindow *pParent )
{
	SButtonGroupID id( nID, pParent );
	if ( !buttonGroups[id] )
		buttonGroups[id] = new CButtonGroup();
	return buttonGroups[id];
}

void CWindowScreen::SetScreenSize( const CTRect<float> &rcScreen )
{
	CWindow::Reposition( rcScreen );
}

void CWindowScreen::SetGView( NGScene::I2DGameView *_p2DGameView )
{
	p2DGameView = _p2DGameView;
	UpdateResolution();
}

void CWindowScreen::Set2DGView( NGScene::I2DGameView *_p2DGameView )
{
	p2DGameView = _p2DGameView;
}

void CWindowScreen::Load( const struct NDb::SUIDesc *pDesc, IProgrammedReactionsAndChecks *_pReactionsAndChecks )
{
	CUIFactory::SetScreenDuringLoad( this );
	pReactionsAndChecks = _pReactionsAndChecks;
	CWindow::InitStatic();
	
	//const NDb::SWindowScreen *pStats = NDb::Get<NDb::SWindowScreen>( nDBID );
	InitByDesc( pDesc );

	CTRect<float> rcScreen;
	const CVec2 vScreenSize( 1024, 768 );
	rcScreen.x1 = 0;
	rcScreen.x2 = vScreenSize.x;
	rcScreen.y1 = 0;
	rcScreen.y2 = vScreenSize.y;
	SetScreenSize( rcScreen );

	CWindow::Init();
	InitScreen();
	// init tooltips
	SetTooltipContext( 0 );
	CUIFactory::SetScreenDuringLoad( 0 );
}

void CWindowScreen::SetReactionsAndChecks( IProgrammedReactionsAndChecks *_pReactionsAndChecks )
{
	pReactionsAndChecks = _pReactionsAndChecks;
}

void CWindowScreen::OnGetFocus( const bool bFocus )
{
	SetFocus( bFocus );
	bIsScreenFocused = bFocus;
	if ( bFocus )
		InitSingletonWindows();
	else
		tooltips.HideTooltip();
}

void CWindowScreen::InitSingletonWindows()
{
	// add console as one of children
	IWindow * pConsole = Singleton<IDebugSingleton>()->GetConsole();
	if ( pConsole )
	{
		if ( !GetChild( pConsole->GetName(), false ) )
		{
			AddChild( pConsole, true );
			pConsole->Init();
		}
	}

	IWindow *pDebug = Singleton<IDebugSingleton>()->GetDebug();
	if ( pDebug )
	{
		if ( !GetChild( pDebug->GetName(), false ) )
			AddChild( pDebug, true );
		pDebug->Init();
	}

	IWindow *pStatSystem = dynamic_cast<IWindow*>( Singleton<IDebugSingleton>()->GetStatsWindow() );
	if ( pStatSystem )
	{
		if ( !GetChild( pStatSystem->GetName(), false ) )
			AddChild( pStatSystem, true );
		pStatSystem->Init();
	}
}

void CWindowScreen::UndoStateCommandSequence( const string &szCmdSeq )
{
	// откатим незаконченную прямую последовательность
	for ( CStateSequiences::iterator it = stateSequiences.begin(); it != stateSequiences.end(); ++it )
	{
		if ( it->GetName() == szCmdSeq ) 
		{
			it->Reverse();
			it->SetContext( pContext );
			it->FastForward( 100000000, this );
			stateSequiences.erase( it );
			return;
		}
	}
	// уберем законченную прямую последовательность
	for ( CStateSequiences::iterator it = finishedAnimations.begin(); it != finishedAnimations.end(); ++it )
	{
		if ( it->GetName() == szCmdSeq ) 
		{
			it->Reverse();
			it->SetContext( pContext );
			it->FastForward( 100000000, this );
			finishedAnimations.erase( it );
			return;
		}
	}
}

void CWindowScreen::RegisterEffect( const string &szEffect, const vector<CDBPtr<NDb::SUIStateBase> > &cmds, const bool bReversable )
{
	commandSequiences[szEffect].commands = cmds;
	commandSequiences[szEffect].bReversable = bReversable;
}
void CWindowScreen::RegisterEffect( const string &szEffect, const NDb::SUIStateSequence &cmds )
{
	commandSequiences[szEffect] = cmds;
}

void CWindowScreen::RegisterReaction( const string &szReactionKey, struct IMessageReactionB2 *pReaction )
{
	messageReactions.Register( szReactionKey, pReaction );
}

void CWindowScreen::Segment( const int timeDiff )
{
	ProcessStateSequiences( timeDiff );
	/*
	{
	// message handler test
	SBUIMessage msg;
	msg.szMessageID = "UI_SHOW_WINDOW";
	ProcessMessage( msg );
	}*/
	for ( CSegmentObjs::iterator it = segmentObjs.begin(); it != segmentObjs.end(); ++it )
		(*it)->Segment( timeDiff );

	if ( bIsScreenFocused )
		tooltips.Segment( timeDiff, this );
}

void CWindowScreen::ProcessStateSequiences( const int timeDiff )
{
	list<CWindowAnimationID> resumed;

	// choose states that are finished animations
	for ( CStateSequiences::iterator ss = stateSequiences.begin(); ss != stateSequiences.end(); )
	{
		CStates &states = *ss;

		if ( states.IsEnd() )
		{
			CWindowAnimationID id = states.GetID();
			CWatingForAnimation::const_iterator wating = watingForAnimation.find( id );
			const bool bWaitForAnim = wating != watingForAnimation.end();
			if ( bWaitForAnim )
				resumed.push_back( id );

			if ( states.IsToBeDeleted() )
				ss = stateSequiences.erase( ss );
			else																// animation finished, but may be reversed.
				finishedAnimations.splice( finishedAnimations.end(), stateSequiences, ss++ );
		}
		else
			++ss;
	}

	// move resumed to running states.
	while ( !resumed.empty() )
	{
		CWatingForAnimation::const_iterator wating = watingForAnimation.find( resumed.front() );
		animationIDs.Return( resumed.front().first );
		stateSequiences.push_back( wating->second );
		resumed.pop_front();
	}

	// process segments to all active states.
	for ( CStateSequiences::iterator ss = stateSequiences.begin(); ss != stateSequiences.end(); )
	{
		if ( !(*ss).IsEnd() )
		{
			(*ss).Segment( timeDiff, this );

			if ( (*ss).IsEnd() )
			{
				if ( (*ss).IsToBeDeleted() )
					ss = stateSequiences.erase( ss );
				else																// animation finished, but may be reversed.
					finishedAnimations.splice( finishedAnimations.end(), stateSequiences, ss++ );
				continue;
			}
		}
		++ss;
	}
}

void CWindowScreen::RegisterToSegment( struct IWindow *pWnd, const bool bRegister )
{
	if ( bRegister )
		segmentObjs.insert( pWnd );
	else
		segmentObjs.remove( pWnd );
}

bool CWindowScreen::IsRegisteredToSegment( struct IWindow *pWnd ) const
{
	return segmentObjs.find( pWnd ) != segmentObjs.end(); 
}

void CWindowScreen::RunAnimationSequienceBack( const int nAnimationID )
{
	if ( 0 == nAnimationID ) return;

	// first search in finished animations
	for ( CStateSequiences::iterator ss = finishedAnimations.begin(); ss != finishedAnimations.end(); )
	{
		if ( ss->GetID().first == nAnimationID )
		{
			ss->Reverse();
			StartEffectAndDeleteIfNeeded( &ss, &stateSequiences, &finishedAnimations, this );
			return;
		}
		else
			++ss;
	}

	// then find animation and try to reverse
	for ( CStateSequiences::iterator ss = stateSequiences.begin(); ss != stateSequiences.end(); ++ss )
	{
		if ( ss->GetID().first == nAnimationID )
		{
			ss->Reverse();
			ss->Segment( 1, this ); // advance a little (to run all imidiate reactions)
			if ( ss->IsEnd() )
				ss = stateSequiences.erase( ss );
			return;
		}
	}
	NI_ASSERT( false, "trying to reverse nonregistered animation" );
}

WORD CWindowScreen::GetKeyboardFlags() const
{
	WORD wRes = (WORD)EKF_NONE;
	if ( bindShift.IsActive() )
		wRes |= (WORD)EKF_SHIFT;
	if ( bindCtrl.IsActive() )
		wRes |= (WORD)EKF_CTRL;
	if ( bindCtrl.IsActive() )
		wRes |= (WORD)EKF_CTRL;
	return wRes;
}

int CWindowScreen::RunAnimationSequienceForward( const NDb::SUIStateSequence &seq, class CWindow *pWindow )
{
	if ( seq.commands.empty() ) return 0;
	const int nID = animationIDs.Get();
	stateSequiences.push_front( CStates( seq, "", seq.bReversable, GetKeyboardFlags() ) );
	CStateSequiences::iterator ss = stateSequiences.begin();
	ss->SetAnimatedWindow( nID, pWindow );
	StartEffectAndDeleteIfNeeded( &ss, &stateSequiences, &finishedAnimations, this );
	return nID;
}

void CWindowScreen::RunStateCommandSequience( const string &szCmdSeq, IWindow *_pSequenceParent, SWindowContext *pContext, const bool bForward, const int nAnimationToWait )
{
	if ( CDynamicCast<CWindow> pSequenceParent = _pSequenceParent )
	{
		if ( 0 == nAnimationToWait )
			RunStateCommandSequienceImmidiate( szCmdSeq, pSequenceParent, pContext, bForward );
		else
		{
			CCommandSequiences::const_iterator cs = commandSequiences.find( szCmdSeq );
			NI_ASSERT( cs != commandSequiences.end(), StrFmt( "unknown commad sequience number %s", szCmdSeq.c_str() ) );
			if ( cs != commandSequiences.end() )
			{
				const NDb::SUIStateSequence &seq = cs->second;
				// register to wait
				CWindowAnimationID id;
				id.first = nAnimationToWait;
				id.second = pSequenceParent;
				watingForAnimation[id] = CStates( seq, szCmdSeq, seq.bReversable, GetKeyboardFlags() );
			}
		}
	}
}

void CWindowScreen::RunStateCommandSequienceImmidiate( const string &szCmdSeq, CWindow *pSequenceParent, SWindowContext *pContext, const bool bForward )
{
	if ( szCmdSeq.empty() ) 
		return;

	CCommandSequiences::const_iterator cs = commandSequiences.find( szCmdSeq );
	NI_ASSERT( cs != commandSequiences.end(), StrFmt( "unknown commad sequience number \"%s\"", szCmdSeq.c_str() ) );
	
	if ( bForward )
	{
		bool bFound = false;
		// find effect with the same name. maybe we need only to reverse
		// 2 effects with same name and same direction are not allowed
		for ( CStateSequiences::iterator it = stateSequiences.begin(); it != stateSequiences.end(); ++it )
		{
			if ( szCmdSeq == it->GetName() ) 
			{
				bFound = true;
				if ( !it->IsForward() )							// reversed one found
					it->Reverse();
			}
		}
		// run fresh effect
		if ( !bFound && cs != commandSequiences.end() )
		{
			const NDb::SUIStateSequence &seq = cs->second;
			stateSequiences.push_front( CStates( seq, szCmdSeq, seq.bReversable, GetKeyboardFlags() ) );
			CStateSequiences::iterator ss = stateSequiences.begin();
			ss->SetNotifySink( pSequenceParent );
			ss->SetContext( pContext );
			StartEffectAndDeleteIfNeeded( &ss, &stateSequiences, &finishedAnimations, this );
		}
	}
	else
	{
		bool bFound = false;
		// find runned effect with this name and reverse it
		for ( CStateSequiences::iterator it = stateSequiences.begin(); it != stateSequiences.end(); ++it )
		{
			if ( szCmdSeq == it->GetName() ) 
			{
				NI_ASSERT( !bFound, StrFmt( "duplicate sequience to reverse found in stateSequieces \"%s\"", szCmdSeq.c_str()) );
				it->Reverse();
				it->SetContext( pContext );
				StartEffectAndDeleteIfNeeded( &it, &stateSequiences, 0, this );
				bFound = true;
			}
		}
		for ( CStateSequiences::iterator it = finishedAnimations.begin(); it != finishedAnimations.end(); )
		{
			if ( szCmdSeq == it->GetName() ) 
			{
				NI_ASSERT( !bFound, StrFmt( "duplicate sequience to reverse found in finishedAnimations \"%s\"", szCmdSeq.c_str() ) );
				it->Reverse();
				it->SetContext( pContext );
				it->Segment( 1, this ); // advance a little (to run all imidiate reactions)
				if ( it->IsEnd() )
					it = finishedAnimations.erase( it );
				else
          stateSequiences.splice( stateSequiences.end(), finishedAnimations, it++ );
				bFound = true;
			}
			else
				++it;
		}
		NI_ASSERT( bFound, StrFmt( "sequience \"%s\" not found in reversable list", szCmdSeq.c_str() ) );
	}
}

void CWindowScreen::SetWindowText( const string &szWindowName, const wstring &szText )
{
	CWindow *pChild = GetDeepChild( szWindowName );
	NI_ASSERT( pChild != 0, StrFmt( "cannot find deeper child \"%s\"", szWindowName.c_str() ) );
	if ( pChild )
	{
		ITextView *pText = dynamic_cast<ITextView*>( pChild );
		NI_ASSERT( pText != 0, StrFmt( "attemt to set text \"%s\" to window that doesn't have text \"%d\"", NStr::ToMBCS(szText.c_str()).c_str(), szWindowName.c_str() ) );
		if ( pText )
			pText->SetText( szText );
	}
}

bool CWindowScreen::RunReaction( const string &szSender, const string &szReactionName )
{
	return messageReactions.Execute( szSender, szReactionName, this, pReactionsAndChecks, GetKeyboardFlags() );
}

bool CWindowScreen::RunReaction( const string &szSender, const NDb::SUIDesc *pReaction )
{
	return messageReactions.Execute( szSender, pReaction, this, pReactionsAndChecks, GetKeyboardFlags() );
}

void CWindowScreen::SetTooltipContext( const int nContext )
{
	tooltips.SetTooltipContext( nContext, this );
}

IWindow *CWindowScreen::CreateTooltipWindow( const wstring &wszTooltipText, IWindow *pTooltipOwner )
{
	return tooltips.CreateTooltipWindow( wszTooltipText, pTooltipOwner, this );
}

void CWindowScreen::InitScreen()
{
	for ( CButtonGroups::iterator it = buttonGroups.begin(); it != buttonGroups.end(); ++it )
	{
		it->second->Init();
		dynamic_cast_ptr<CWindow*>(it->first.pParent)->RepositionChildren();
	}
}

void CWindowScreen::SetGView( NGScene::I2DGameView *_p2DGameView, NGScene::IGameView *_pGameView, NGScene::IGameView *_pInterface3DView )
{
	p2DGameView = _p2DGameView;
	pInterface3DView = _pInterface3DView;
	UpdateResolution();
}

int CWindowScreen::operator&( struct IBinSaver &saver )
{
	saver.Add( 1, static_cast<CWindow*>( this ) );
	saver.Add( 2, &stateSequiences );
	saver.Add( 3, &messageReactions );
	saver.Add( 4, &pReactionsAndChecks );
	saver.Add( 6, &commandSequiences );
	saver.Add( 7, &tooltips );
	saver.Add( 8, &buttonGroups );
	saver.Add( 9, &pInstance );
	saver.Add( 10, &watingForAnimation );
	saver.Add( 11, &animationIDs );
	saver.Add( 15, &segmentObjs );
	//saver.Add( 16, &p2DGameView );
	saver.Add( 17, &finishedAnimations );
	saver.Add( 18, &tabOrder );
	return 0;
}

void CWindowScreen::AfterLoad()
{
	nMouseButtonState = 0;
	InitSingletonWindows();
	InitStatic();
	InitScreen();
	CWindow::AfterLoad();
}

IWindow* CWindowScreen::AddElement( const struct NDb::SUIDesc *pDesc )
{
	//CRAP{ FIND OUT WHO USE THIS
	return CUIFactory::MakeWindow( pDesc );
	//CRAP}
}

const wstring& CWindowScreen::GetTextEntry( const string &szName ) const
{
	static wstring empty;
	if ( !pInstance )
		return empty;

	for ( int i = 0; i < pInstance->relatedTexts.size(); ++i )
	{
		if ( pInstance->relatedTexts[i].szName == szName && CHECK_TEXT_NOT_EMPTY_PRE( pInstance->relatedTexts[i]., Text ) )
			return GET_TEXT_PRE( pInstance->relatedTexts[i]., Text );
	}
	return empty;
}

bool CWindowScreen::ActivateNextInTabOrder()
{
	CWindow* pFocused = FindFocusedWindow();
	int nFocused = -1;
	if ( pFocused && tabOrder.Size() != 0 )
	{
		for ( int i = 0; i < tabOrder.Size(); ++i )
		{
			if ( pFocused == tabOrder[i].first )
			{
				nFocused = i;
			}
		}
	}
	if ( nFocused != -1 ) // try to set focus to another window
	{
		const int nNewFocused = nFocused + 1;
		for ( int i = 0; i < tabOrder.Size(); ++i )
		{
			CWindow * pTry = tabOrder[(i+nNewFocused)%tabOrder.Size()].first;
			if ( pTry->IsVisible() && pTry->IsEnabled() )
			{
				pTry->SetFocus( true );
				return true;
			}
		}
	}
	return false;
}

void CWindowScreen::RegisterTabOrder( IWindow * pWindow, int nTabOrder )
{
	if ( nTabOrder != -1 )
		tabOrder.Push( pair<CObj<CWindow>, int>(dynamic_cast<CWindow*>(pWindow), nTabOrder) );
}


