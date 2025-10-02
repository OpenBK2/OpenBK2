// Window.cpp: implementation of the CWindow class.// Window.cpp: implementation of the CWindow class.
//


#include "stdafx.h"
#include "BackgroundSimpleTexture.h"
#include "ForegroundTextString.h"
#include "Tools.h"
#include "UIScreen.h"
#include "Window.h"
#include "uiinternal.h"
#include "window.h"
#include "WindowTooltip.h"
#include "Misc/PlaneGeometry.h"
#include "System/Commands.h"
#include "System/Text.h"
#include "3DMotor/DBScene.h"


int CHECK_DUPLICATE_CHILDREN;
UI_EXPORT int CHECK_UI_TEXTURES_INSTANT_LOAD;
bool s_bUICommonShowWarnings;

BASIC_REGISTER_CLASS(CWindow);

void CUIMORegConttainer::AddRawObserver( const string &szMsgName, IGMObserver *pObserver )
{
	CUIFactory::RegisterMessage( szMsgName );
	eventRegisters.insert( eventRegisters.begin(), NInput::CGMOReg(szMsgName, pObserver) );
}

// SWindowCompare

bool SWindowCompare::operator()( const CObj<CWindow> &o1, const CObj<CWindow> &o2 ) const
{ 
	return o1->GetPriority() < o2->GetPriority(); 
}

int SAnimationIDHash::operator()( const CWindowAnimationID &p ) const
{
	SDefaultPtrHash h;
	return p.first + h( p.second );
}


// CWindow




CWindow::CWindow() : pClickNotify( 0 ), 
	vScreenPos( VNULL2 ), bIsModal ( false ), bFocused( false ),
	bDelayChildRemove( false ),
	fFadeValue( 1.0f ),
	fInternalFadeValue( 1.0f ),
	nIDForMLHandler( -1 )
{
	delayedReposition.second = false;
}

int CWindow::operator&( IBinSaver &saver )
{
//	saver.Add( 1, &pTextString );
	saver.Add( 3, &pBackground );					// may be 0
	saver.Add( 4, &pParent );									// parent window.
	saver.Add( 5, &pFocusedChild );									// child that has keyboard focus
	saver.Add( 6, &pHighlighted );							// window currently under mouse cursor
	saver.Add( 7, &pressed );		// pressed with each mouse button
	saver.Add( 8, &vScreenPos );
	saver.Add( 9, &drawOrder );
	saver.Add( 10, &children );
	saver.Add( 12, &pForeground );
	saver.Add( 14, &pContext );
	saver.Add( 15, &pShared );
	saver.Add( 16, &pWindowStats );
	saver.Add( 18, &bIsModal );
	saver.Add( 19, &bFocused );
	saver.Add( 20, &pPlacedText );		
	saver.Add( 21, &nOutlineType );
	saver.Add( 23, &fFadeValue );
	saver.Add( 24, &fInternalFadeValue );
	saver.Add( 25, &delayedReposition );
	saver.Add( 26, &nIDForMLHandler );
	saver.Add( 27, &pCustomTooltip );
	saver.Add( 28, &wszCustomTooltip );
	return 0;
}

void CWindow::AfterLoad()
{
	for ( int i = 0; i < drawOrder.Size(); ++i )
		drawOrder[i]->AfterLoad();
}

int CWindow::RunAnimationAndCommands( const NDb::SUIStateSequence &seq, const string &szCommanEffect, 
																			 const bool bWaitOnGraphics, const bool bForward )
{
	IScreen *pScreen = GetScreen();
	if ( pScreen )
	{
		const int nID = pScreen->RunAnimationSequienceForward( seq, this );
		const int nWaitID = bWaitOnGraphics ? nID  : 0;
		if ( !szCommanEffect.empty() )
		{
			pScreen->RunStateCommandSequience( szCommanEffect, this, pContext, bForward, nWaitID );
		}
		return nID;
	}
	return 0;
}

int CWindow::GetOptimalWidth() const
{
/*	if ( !pTextString ) 
		return 0;
	return 
		pTextString->GetOptimalWidth();*/
	if ( !pPlacedText )
		return 0;
	return pPlacedText->GetOptimalWidth();
}

void CWindow::SetOutline( const CDBID &_outlineType )
{
	CTRect<int> rc = GetPlacement();
	CVec2 vAdd = CIconOutliner::GetSizeAdd();
	SetPlacement( rc.left-vAdd.x/4, rc.top-vAdd.y/4, rc.Width() + vAdd.x/2, rc.Height() + vAdd.y/2, EWPF_ALL );
	nOutlineType = _outlineType;	
}

void CWindow::SetTextString( const wstring &szText )
{
/*	NI_ASSERT( pTextString != 0, "attempt to set text to window that doesn't allow it" );
	if ( pTextString )
		pTextString->SetText( szText );*/
	if ( pPlacedText )
	{
		pPlacedText->Reposition( GetWindowRect() );
		pPlacedText->SetText( szText );
	}
}

const wchar_t * CWindow::GetTextString() const 
{ 
//	NI_ASSERT( pTextString, "text string not found" );
/*	if ( !pTextString )
		return L"";
	return pTextString->GetText().c_str();  */
	if ( !pPlacedText )
		return L"";
	return pPlacedText->GetText().c_str();
}

void CWindow::GameMessageSink( const struct SGameMessage &msg, int nMessageType )
{
	NI_ASSERT( nMessageType < pWindowStats->gameMessageReactions.size(), "registed on unknown message and recieved it" );

	// CRAP{ wrong default value
	string szLogicalReaction =
		pWindowStats->gameMessageReactions[nMessageType].szLogicalReaction == " " ? "" : pWindowStats->gameMessageReactions[nMessageType].szLogicalReaction;
	// CRAP}

	RunAnimationAndCommands( pWindowStats->gameMessageReactions[nMessageType].visualReaction,
													szLogicalReaction,
													pWindowStats->gameMessageReactions[nMessageType].bWaitVisual,
													pWindowStats->gameMessageReactions[nMessageType].bForward );
}

void CWindow::RegisterObservers()
{
	for ( int i = 0; i < drawOrder.Size(); ++i )
		drawOrder[i]->RegisterObservers();

	// register own observers
	for ( int i = 0; i < pWindowStats->gameMessageReactions.size(); ++i )
		AddObserver( pWindowStats->gameMessageReactions[i].szGameMessage, &CWindow::GameMessageSink, i );
}

void CWindow::SetBackground( IWindowPart *_pBackground )
{	
	pBackground = _pBackground; 
	if ( pBackground )
		pBackground->SetPos( vScreenPos, GetInstance()->placement.size.Get() );
}

void CWindow::SetForeground( IWindowPart *_pForeground )
{
	pForeground = _pForeground;
	if ( pForeground )
		pForeground->SetPos( vScreenPos, GetInstance()->placement.size.Get() );
}

const wstring& CWindow::GetDBFormatText() const
{
	static wstring szEmpty;

	if ( const NDb::SForegroundTextString *pForeground = GetInstance()->pTextString )	
	{
		if ( pForeground->pShared && CHECK_TEXT_NOT_EMPTY_PRE(pForeground->pShared->,FormatString) )
			return GET_TEXT_PRE(pForeground->pShared->,FormatString);
	}

	return szEmpty;
}

const wstring& CWindow::GetDBInstanceText() const
{
	static wstring szEmpty;

	if ( const NDb::SForegroundTextString *pForeground = GetInstance()->pTextString )
	{
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pForeground->,TextString) )
			return GET_TEXT_PRE(pForeground->,TextString);
	}
		
	return szEmpty;
}

wstring CWindow::GetDBText() const
{
	return GetDBFormatText() + GetDBInstanceText();
}

const NDb::SWindowPlacement* CWindow::GetDBTextPlacement() const
{
	if ( const NDb::SForegroundTextString *pForeground = GetInstance()->pTextString )	
	{
		if ( pForeground->pShared )
			return &pForeground->pShared->position;
	}
	return 0;
}

void CWindow::SetTextPlacement( const struct NDb::SWindowPlacement &placement )
{
	if ( pPlacedText )
		pPlacedText->SetPlacement( placement );
}

struct IScreen* CWindow::GetScreen()
{
	if ( GetParent() == 0 )
	{
		if ( CUIFactory::GetScreenDuringLoad() )
			return CUIFactory::GetScreenDuringLoad();
		else
			return dynamic_cast<CWindowScreen*>( this );
	}
	else 
		return GetParent()->GetScreen();
}

void CWindow::SetFocus( const bool bFocus )
{
	if ( bFocus && !IsFocused() )
		RemoveAnyFocus();
		
	if ( GetParent() )
	{
		if ( bFocus )
			GetParent()->ProcessFocused( this );
		else
			GetParent()->ProcessUnfocused( this );
	}

	bFocused = bFocus;
}

CWindow* CWindow::FindFocusedWindow()
{
	// если не находимся в ветке с фокусированным окном, ищем его по пути к корню
	CWindow *pWnd = this;
	while ( true )
	{
		if ( pWnd->bFocused )
			return pWnd;
			
		if ( !pWnd->GetParent() )
			break;
			
		pWnd = pWnd->GetParent();
	}
		
	// если есть фокусированное окно, спускаемся по ветке до конечного
	while ( pWnd->pFocusedChild )
	{
		pWnd = pWnd->pFocusedChild;
	}
	
	return pWnd->bFocused ? pWnd : 0;
}

void CWindow::RemoveAnyFocus()
{
	CWindow *pWnd = FindFocusedWindow();
	if ( pWnd )
		pWnd->SetFocus( false );
}

void CWindow::CheckRemoveFocus( CWindow *pClickedWnd )
{
	if ( !pClickedWnd )
		return;
		
	CWindow *pFocused = FindFocusedWindow();
	if ( !pFocused )
		return;
		
	if ( pFocused == pClickedWnd )
		return;

	// не будем удалять фокус, если кликнули в фокусированное окно или одного из его потомков
	CWindow *pWnd = pClickedWnd;
	while ( pWnd )
	{
		if ( pWnd->IsRelatedFocus( pFocused ) )
			return;
			
		pWnd = pWnd->GetParent();
	}

	pFocused->SetFocus( false );
}

void CWindow::ProcessFocused( CWindow *pChild )
{
	pFocusedChild = pChild;
	if ( GetParent() )
		GetParent()->ProcessFocused( this );
}

void CWindow::ProcessUnfocused( CWindow *pChild )
{
	pFocusedChild = 0;
	if ( GetParent() )
		GetParent()->ProcessUnfocused( this );
}

CWindow* CWindow::GetRoot()
{
	CWindow *pWnd = this;
	while ( pWnd->GetParent() )
	{
		pWnd = pWnd->GetParent();
	}
	return pWnd;
}

void CWindow::OnChangeVisibility( bool bShow )
{
	for ( int i = 0; i < drawOrder.Size(); ++i )
	{
		CWindow *pWnd = drawOrder[i];
		pWnd->OnChangeVisibility( bShow );
	}
}

void CWindow::ShowWindow( const bool bShow )
{
	bool bVisible = IsVisible();
	GetInstance()->bVisible = bShow;
	if ( bVisible != bShow )
	{
		if ( delayedReposition.second )
		{
			Reposition( delayedReposition.first );
			delayedReposition.second = false;
		}
		// notify all windows with delayed reposition 
		// that they have to do reposition now
		else
			RepositionChildren();
		OnChangeVisibility( bShow );
	}
}

bool CWindow::IsVisible() const 
{ 
	if ( GetInstance()->bVisible )
	{
		if ( pParent )
			return pParent->IsVisible();
		else
			return true;
	}
	else
		return false;
}

int CWindow::GetPriority() const 
{ 
	return GetInstance()->nPriority; 
}

void CWindow::SetPriority( int n )
{
	GetInstance()->nPriority = n;
}

const string& CWindow::GetName() const 
{ 
	return GetInstance()->szName; 
}

void CWindow::SetName( const string &_szName )
{
	GetInstance()->szName = _szName;
}

void CWindow::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	pWindowStats = checked_cast<const NDb::SWindow*>( _pDesc );
	pShared = checked_cast_ptr<const NDb::SWindowShared *>( pWindowStats->pShared );

	if ( pShared )
	{
		for ( vector<CDBPtr<NDb::SUIDesc> >::const_iterator it = pShared->children.begin(); it != pShared->children.end(); ++it )
		{
			IWindow *pWindow = CUIFactory::MakeWindow( *it );
			AddChild( pWindow, false );
		}
		pBackground = CUIFactory::MakeWindowPart( pShared->pBackground );

	//	if ( GetInstance()->pTextString )
	//		pTextString =	static_cast<CForegroundTextString*>( CUIFactory::MakeWindowPart( GetInstance()->pTextString ) );
		pPlacedText = new CPlacedText(); // always presents
		SetTextString( GetDBText() );
		if ( const NDb::SWindowPlacement *pPlacement = GetDBTextPlacement() )
			SetTextPlacement( *pPlacement );

		pForeground =	CUIFactory::MakeWindowPart( pShared->pForeground );

		GetInstance()->placement.position.Merge( pShared->placement.position.Get() );
		GetInstance()->placement.lowerMargin.Merge( pShared->placement.lowerMargin.Get() );
		GetInstance()->placement.upperMargin.Merge( pShared->placement.upperMargin.Get() );
		GetInstance()->placement.size.Merge( pShared->placement.size.Get() );
		GetInstance()->placement.verAllign.Merge( pShared->placement.verAllign.Get() );
		GetInstance()->placement.horAllign.Merge( pShared->placement.horAllign.Get() );
	}
}

void CWindow::Init()
{
//	if ( pTextString )
//		pTextString->Init();
	if ( pPlacedText )
		pPlacedText->Init();
	if ( pForeground )
		pForeground->Init();
	if ( pBackground )
		pBackground->Init();

	for ( int i = 0; i < drawOrder.Size(); ++i )
		drawOrder[i]->Init();

	if ( !pWindowStats->bEnabled )
		Enable( false );
}

void CWindow::Reposition( const CTRect<float> &parentRect )
{
	if ( !IsVisible() )
	{
		delayedReposition.first = parentRect;
		delayedReposition.second = true;
		return;
	}

	const CVec2 vParentPos( parentRect.left, parentRect.top );
	const CVec2 vParentSize( parentRect.right-parentRect.left, parentRect.bottom - parentRect.top );
	// calc position according to parent

	NDb::SWindowPlacement &placement = GetInstance()->placement;

	NUITools::ApplyWindowAlign( placement.horAllign.Get(), vParentPos.x, vParentSize.x,
															placement.position.Get().x,
															placement.lowerMargin.Get().x, placement.upperMargin.Get().x,
															&placement.size.first.x, 
															&vScreenPos.x );

	NUITools::ApplyWindowAlign( placement.verAllign.Get(), vParentPos.y, vParentSize.y,
															placement.position.Get().y,
															placement.lowerMargin.Get().y, placement.upperMargin.Get().y,
															&placement.size.first.y, 
															&vScreenPos.y );
	if ( pBackground )
		pBackground->SetPos( vScreenPos, placement.size.Get() );
	
//	if ( pTextString )
//		pTextString->SetPos( vScreenPos, placement.size.Get() );
	if ( pPlacedText )
		pPlacedText->Reposition( GetWindowRect() );

	if ( pForeground )
		pForeground->SetPos( vScreenPos, placement.size.Get() );
	
	RepositionChildren();
}

void CWindow::RepositionChildren( IWindow *_pWindow )
{
	CWindow * pWindow = dynamic_cast<CWindow*>( _pWindow );

	CTRect<float> rCurrent = GetWindowRect();
	if ( pWindow )
	{
		if ( children.find( pWindow ) != children.end() )
			pWindow->Reposition( rCurrent );
	}
	else
	{
		for ( int i = 0; i < drawOrder.Size(); ++i )
			drawOrder[i]->Reposition( rCurrent );
	}
}

void CWindow::GetPlacement( int *pX, int *pY, int *pSizeX, int *pSizeY ) const
{
	if ( pX )
		*pX = GetInstance()->placement.position.Get().x;
	if ( pY )
		*pY = GetInstance()->placement.position.Get().y;
	if ( pSizeX )
		*pSizeX = GetInstance()->placement.size.Get().x;
	if ( pSizeY )
		*pSizeY = GetInstance()->placement.size.Get().y;
}

CTRect<int> CWindow::GetPlacement() const
{
	int x = GetInstance()->placement.position.Get().x;
	int y = GetInstance()->placement.position.Get().y;
	int w = GetInstance()->placement.size.Get().x;
	int h = GetInstance()->placement.size.Get().y;
	return CTRect<int>( x, y, x+w, y+h );
}

void CWindow::FillWindowRect( CTRect<float> *pRect ) const
{
	pRect->top = vScreenPos.y;
	pRect->left = vScreenPos.x;
	pRect->right = GetInstance()->placement.size.Get().x + vScreenPos.x;
	pRect->bottom = GetInstance()->placement.size.Get().y + vScreenPos.y;
}

CTRect<float> CWindow::GetWindowRect() const
{
	const NDb::SWindow* pInstance = GetInstance();
	return CTRect<float>( vScreenPos.x, vScreenPos.y, pInstance->placement.size.Get().x + vScreenPos.x, pInstance->placement.size.Get().y + vScreenPos.y );
}

void CWindow::SetPlacement( const CTRect<int> &rc, const DWORD flags )
{
	SetPlacement( rc.x1, rc.y1, rc.Width(), rc.Height(), flags );
}

void CWindow::SetPlacement( const float x, const float y, const float sizeX, const float sizeY, const DWORD flags ) 
{
	if ( flags & EWPF_POS_X )
	{
		vScreenPos.x += x - GetInstance()->placement.position.Get().x;
		GetInstance()->placement.position.first.x = x;
	}
	if ( flags & EWPF_POS_Y )
	{
		vScreenPos.y += y - GetInstance()->placement.position.Get().y;
		GetInstance()->placement.position.first.y = y;
	}
	if ( flags & EWPF_SIZE_X )
		GetInstance()->placement.size.first.x = sizeX;
	if ( flags & EWPF_SIZE_Y )
		GetInstance()->placement.size.first.y = sizeY;

	if ( pBackground )
		pBackground->SetPos( vScreenPos, GetInstance()->placement.size.Get() );
//	if ( pTextString )
//		pTextString->SetPos( vScreenPos, GetInstance()->placement.size.Get() );
	if ( pPlacedText )
		pPlacedText->Reposition( GetWindowRect() );
	if ( pForeground )
		pForeground->SetPos( vScreenPos, GetInstance()->placement.size.Get() );
	
	RepositionChildren();
}


void CWindow::AddChild( IWindow *pWnd, bool bDoReposition )
{
	if ( !pWnd )
		return;

	CPtr<CWindow> pWindow = dynamic_cast<CWindow*>(pWnd);
	
	if ( children.find( pWindow ) != children.end() )
		return;

	if ( drawOrder.GetReserved() <= drawOrder.Size() + 1 )
		drawOrder.Reserve( drawOrder.Size() + Max( 10, drawOrder.Size() ) );
	drawOrder.Push( pWindow.GetPtr() ); 
	pWindow->SetParent( this );
	children.insert( pWindow );
	if ( bDoReposition )
		RepositionChildren( pWnd );
}

void CWindow::RemoveChild( IWindow *_pChild )
{
	CWindow *pChild = dynamic_cast<CWindow*>(_pChild);
	if ( bDelayChildRemove )
		removedChildren.push_back( pChild );
	else
	{
		CChildren::iterator it = children.find( pChild );
		if ( it != children.end() )
		{
			children.erase( it );
			drawOrder.Remove( pChild );
		}
	}
}

void CWindow::RemoveChild( const string &szChildName )
{
	CWindow *pChild = dynamic_cast<CWindow*>(GetChild( szChildName, false ));
	RemoveChild( pChild );
}

void CWindow::Close()
{
	if ( GetParent() )
		GetParent()->RemoveChild( this );
	else
		ASSERT( 0 );
}

int CWindow::CheckNamedChildrenCount( const string &szName, bool bRecursive )
{
	int nCount = 0;
	for ( int i = 0; i < drawOrder.Size(); ++i )
	{
		if ( szName == drawOrder[i]->GetName() )
			nCount++;
		if ( bRecursive )
			nCount += drawOrder[i]->CheckNamedChildrenCount( szName, bRecursive );
	}
	//NI_ASSERT( nCount <= 1, StrFmt( "duplicate children name: %s", szName.c_str() ) );
	return nCount;
}

int CWindow::GetNumChildren()
{
	return drawOrder.Size();
}

IWindow *CWindow::GetChild( int nIndex )
{
	if( nIndex<0 || nIndex > drawOrder.Size()-1 )
	{
		NI_ASSERT( 0, "Programmers: check the GetNumChildren() first. And try do not use this at all." );
		return 0;
	}
	return drawOrder[nIndex];
}

IWindow* CWindow::GetChild( const string &_szName, const bool bRecursive )
{
	if ( _szName.empty() ) 
		return 0;
		
	//{ CRAP - wrong window name
	if ( _szName == " " ) 
		return 0;
	//}
	
#ifndef _FINALRELEASE
	if ( CHECK_DUPLICATE_CHILDREN != 0 )
	{
		NI_ASSERT( CheckNamedChildrenCount( _szName, bRecursive ) <= 1, StrFmt( "Duplicate window name %s", _szName.c_str() ) );
	}
#endif
	for ( int i = 0; i < drawOrder.Size(); ++i )
	{
		string dbg = drawOrder[i]->GetName();
		if ( _szName == drawOrder[i]->GetName() )
			return drawOrder[i];
	}

	if ( bRecursive )
	{
		for ( int i = 0; i < drawOrder.Size(); ++i )
		{
			IWindow* pWnd = drawOrder[i]->GetChild( _szName, true );
			if ( pWnd )
				return pWnd;
		}
	}
	return 0;
}

IWindow* CWindow::GetVisibleChild( const string &_szName, const bool bRecursive )
{
	if ( _szName.empty() ) 
		return 0;
	
	for ( int i = 0; i < drawOrder.Size(); ++i )
	{
		if ( drawOrder[i]->IsVisible() && _szName == drawOrder[i]->GetName() )
			return drawOrder[i];
	}
	if ( bRecursive )
	{
		for ( int i = 0; i < drawOrder.Size(); ++i )
		{
			IWindow* pWnd = drawOrder[i]->GetVisibleChild( _szName, true );
			if ( pWnd )
				return pWnd;
		}
	}
	return 0;
}

IWindow* CWindow::GetChild( const int _nTypeID, const int _nID, const bool bRecursive )
{
	for ( int i = 0; i < drawOrder.Size(); ++i )
		if ( drawOrder[i]->GetDesc()->GetTypeID()   == _nTypeID 
			&& drawOrder[i]->GetDesc()->GetRecordID() == _nID )
		{
			return drawOrder[i];
		}
	if ( bRecursive )
	{
		for ( int i = 0; i < drawOrder.Size(); ++i )
		{
			IWindow* pWnd = drawOrder[i]->GetChild( _nTypeID, _nID, true );
			if ( pWnd )
				return pWnd;
		}
	}
	return 0;
}

CWindow* CWindow::GetDeepChild( const string &_szName )
{
	CWindow *pRet = dynamic_cast<CWindow*>(GetChild( _szName, false ));
	if ( !pRet ) // not immidiate child
	{
		// find deeper child
		for ( int i = drawOrder.Size() -1; i >= 0; --i )
			if ( pRet = drawOrder[i]->GetDeepChild( _szName ) )
				return pRet;
	}
	return 0;
}


void CWindow::SetParent( CWindow *_pParent )
{
	if ( pParent && pParent->IsRefValid() )
		pParent->RemoveChild( this );
	pParent = _pParent;
}

bool CWindow::OnButtonDown( const CVec2 &vPos, const int nButton )
{
	if ( !IsEnabled() )
		return false;

	NI_ASSERT( nButton >= 0, StrFmt( "don't understand such buttons %i", nButton) );
	pressed.resize( Max( (int)pressed.size(), nButton + 1 ) );

	CPtr<CWindow> pTmp = pressed[nButton];
	pressed[nButton] = PickInternal( vPos, false );

	bool bHandledByChild = false;
	if ( pressed[nButton] )
		bHandledByChild = pressed[nButton]->OnButtonDown( vPos, nButton );
	if ( !bHandledByChild )
		CheckRemoveFocus( this );
	if ( pTmp && pTmp->IsRefValid() && pressed[nButton] != pTmp )
		pTmp->OnMouseMove( CVec2(-1,-1), 0 );

	// выполним общее уведомление после частных (для содержимого) - порядок важен
	if ( pClickNotify && IsInside( vPos ) )
		pClickNotify->Clicked( this, nButton );
	
	return pressed[nButton] != 0;//bHandledByChild;
}

bool CWindow::OnButtonUp( const CVec2 &vPos, const int nButton )
{
	if ( !IsEnabled() )
		return false;
	NI_ASSERT( nButton >= 0, StrFmt( "don't understand such buttons %i", nButton) );

//	if ( CWindow *pChild = PickInternal( vPos, false ) )
//		return pChild->OnButtonUp( vPos, nButton );

	pressed.resize( Max( (int)pressed.size(), nButton + 1 ) );
	const bool bWasPressed = pressed[nButton] != 0;
	if ( pressed[nButton] )
	{
		pressed[nButton]->OnButtonUp( vPos, nButton );
	}
	if ( pressed.size() > nButton ) //OnButtonUp can spoil the fun!
		pressed[nButton] = 0;
	return bWasPressed;
}

void CWindow::Enable( const bool bEnable )
{ 
	for ( int nButton = 0; nButton < pressed.size(); ++nButton )
	{
		if ( pressed[nButton] )
			pressed[nButton]->OnButtonUp( CVec2(-1,-1), nButton );
	}
	pressed.clear();
	GetInstance()->bEnabled = bEnable; 
}

bool CWindow::OnButtonDblClk( const CVec2 &_vPos, const int nButton )
{
	if ( !IsEnabled() )
		return false;
	if ( pShared->bIgnoreDblClick )
		return OnButtonDown( _vPos, nButton );// && OnButtonUp( _vPos, nButton );

	if ( pClickNotify && IsInside( _vPos ) )
		pClickNotify->DoubleClicked( this, nButton );

	if ( CWindow *pChild = PickInternal( _vPos, false ) )
		return pChild->OnButtonDblClk( _vPos, nButton );

	return false;
}

bool CWindow::OnMouseMove( const CVec2 &_vPos,  const int nMouseState )
{
	if ( MSTATE_FREE == nMouseState )		// no button pressed
	{
		{
			CWindow *pNewHighligted = 0;
			if ( _vPos.x < 0 || _vPos.y < 0 )
			{
				if ( pHighlighted )
					pHighlighted->OnMouseMove( _vPos, nMouseState );
				pHighlighted = 0;
			}
			else if ( !pHighlighted )
			{
				pHighlighted = PickInternal( _vPos, false );
			}
			else if ( (pNewHighligted = PickInternal( _vPos, false )) != pHighlighted )
			{
				if ( pHighlighted )	// remove highlight
					pHighlighted->OnMouseMove( CVec2(-1,-1), nMouseState );
				pHighlighted = pNewHighligted;
			}
		}
		
		if ( pHighlighted )
			pHighlighted->OnMouseMove( _vPos, nMouseState );
	}
	else if ( MSTATE_BUTTON1 & nMouseState )
	{
		for ( int i = drawOrder.Size() - 1; i >= 0; --i )
		{
			if ( !drawOrder[i]->IsTransparent() && drawOrder[i]->IsPickable() )
			{
				if ( drawOrder[i]->OnMouseMove( _vPos, nMouseState ) )
					return true;
			}
		}
	}
	return false;
}
/*

void CWindow::OnChar( const wchar_t chr )
{
	if ( pFocused )
		pFocused->OnChar( chr );
}
*/

bool CWindow::IsPickable() const 
{
	return IsVisible() && !IsTransparent();
}

CWindow* CWindow::PickInternal( const CVec2 &vPos, const bool bRecursive )
{
	if ( !IsPickable() )
		return 0;
	for ( int i = drawOrder.Size() - 1; i >= 0 ; --i )
	{
		CWindow *pWnd = drawOrder[i];
		if ( pWnd->IsModal() )
			return pWnd;

		if ( pWnd->IsPickable() && pWnd->IsInside( vPos ) ) 
		{
			if ( bRecursive )
			{
				CWindow *pPick = pWnd->PickInternal( vPos, bRecursive );
				if ( pPick ) 
					return pPick;
			}
			return pWnd;
		}
	}
	return 0;
}

IWindow *CWindow::Pick( const CVec2 &vPos, const bool bRecursive )
{
	CVec2 vPosOnScreen;
	ScreenToVirtual( vPos, &vPosOnScreen );
	return PickInternal( vPosOnScreen, bRecursive );
}

IWindow *CWindow::DemandTooltip()
{
	if ( pCustomTooltip )
		return pCustomTooltip;

	IScreen *pScreen = GetScreen();
	if ( pScreen == 0 )
		return 0;

	if ( !wszCustomTooltip.empty() )
	{
		pCustomTooltip = pScreen->CreateTooltipWindow( wszCustomTooltip, this );
		wszCustomTooltip = L"";
		return pCustomTooltip;
	}

	if ( CHECK_TEXT_NOT_EMPTY_PRE(GetInstance()->,Tooltip) )
	{
		const wstring wszDbTooltip = GET_TEXT_PRE(GetInstance()->,Tooltip);
		pCustomTooltip = pScreen->CreateTooltipWindow( wszDbTooltip, this );
		return pCustomTooltip;
	}

	return 0;
}

const wstring& CWindow::GetDBTooltipStr() const
{
	if ( CHECK_TEXT_NOT_EMPTY_PRE(GetInstance()->,Tooltip) )
		return GET_TEXT_PRE(GetInstance()->,Tooltip);

	static wstring wszEmpty;
	return wszEmpty;
}

void CWindow::SetTooltip( const wstring &wszTooltip )
{
	pCustomTooltip = 0;
	// can't create tooltip window now because pScreen might be zero, create tooltip later
	wszCustomTooltip = wszTooltip;
}

void CWindow::SetTooltip( IWindow *pTooltipWindow )
{
	wszCustomTooltip = L"";
	pCustomTooltip = pTooltipWindow;
}

void CWindow::SetTooltipIDForMLHandler( int nID )
{
	nIDForMLHandler = nID;
}

int CWindow::GetTooltipIDForMLHandler() const
{
	return nIDForMLHandler;
}

void CWindow::Visit( struct IUIVisitor *pVisitor )
{
	if ( !IsVisible() )
		return;

	float fFade = GetTotalFadeValue();

	// background
	if ( pBackground )
	{
		pBackground->SetFadeValue( fFade );
		if ( !nOutlineType.IsEmpty() )
			pBackground->SetOutline( nOutlineType );
		pBackground->Visit( pVisitor );
	}
	// text
//	if ( pTextString )
//		pTextString->Visit( pVisitor );
	if ( pPlacedText )
	{
		pPlacedText->SetInternalFadeValue( fFade );
		pPlacedText->Visit( pVisitor );
	}

	// children
	for ( int i = 0; i < drawOrder.Size(); ++i )
	{
		CWindow *pWnd = drawOrder[i];
		if ( pWnd->IsVisible() )
		{
			pWnd->SetInternalFadeValue( fFade );
			pWnd->Visit( pVisitor );
		}
	}
	
	// foreground
	if ( pForeground )
	{
		pForeground->SetFadeValue( fFade );
		pForeground->Visit( pVisitor );
	}
}

bool CWindow::IsInside( const CVec2 &vPos ) const
{
	return (vPos.x >= vScreenPos.x && 
					vPos.y >= vScreenPos.y &&
					vPos.x < vScreenPos.x + GetInstance()->placement.size.Get().x &&
					vPos.y < vScreenPos.y + GetInstance()->placement.size.Get().y &&
					IsActiveArea( vPos ));
}

bool CWindow::IsActiveArea( const CVec2 &vPos ) const
{
	const NDb::SWindowShared *pShared = GetSharedDesc();
	if ( pShared->activeArea.empty() )
		return true;
	else
	{
		return CP_OUTSIDE != ClassifyPolygon( pShared->activeArea, vPos - vScreenPos );
	}
}

void CWindow::DelayedChildRemove()
{
	for ( int i = 0; i < removedChildren.size(); ++i )
		drawOrder.Remove( removedChildren[i] );
	removedChildren.clear();
	bDelayChildRemove = false;
}

bool CWindow::ProcessEvent( const struct SGameMessage &msg )
{
	//DEBUG{
#ifdef _DEBUG
	char buf[256];
	strcpy( buf, GetInstance()->szName.c_str() );
#endif
	/*
	static hash_map<string, int> calls;
	if ( NGlobal::GetVar( "c_e", 0 ) == 1 )
	{
		if ( calls.find( msg.szEventName ) == calls.end() )
			calls[msg.szEventName] = 0;
		else		
			++calls[msg.szEventName];
	}
	if ( NGlobal::GetVar( "p_e", 0 ) == 1 )
	{
		NGlobal::SetVar( "p_e", 0 );
		for ( hash_map<string, int>::iterator it = calls.begin(); it != calls.end(); ++it )
		{
			DebugTrace( "%s %d", it->first.c_str(), int(it->second) );
		}
		calls.clear();
	}*/
	//DEBUG}
	if ( IsValid( pFocusedChild ) && pFocusedChild->IsVisible() )
	{
		if ( pFocusedChild->ProcessEvent( msg ) )
			return true;
	}
	if ( priorityEvents.ProcessEvent( msg, this ) )
		return true;
	// children, should work even if windows are removed in progress
	bDelayChildRemove = true;
	for ( int i = drawOrder.Size() - 1; i >= 0; --i )
	{
		CWindow * pWindow = drawOrder[i];
		if ( !pWindow )
			continue;
		if ( pWindow == pFocusedChild )
			continue;
		if ( msg.mMessage.cType != NInput::CT_UNKNOWN && ( !pWindow->IsVisible() || !pWindow->IsEnabled() ) )
			continue;
		if ( pWindow->ProcessEvent( msg ) )
		{
			DelayedChildRemove();
			return true;
		}
	}
	DelayedChildRemove();

	if ( CGMORegContainer::ProcessEvent( msg, this ) )
		return true;
	
	return bIsModal && msg.mMessage.cType !=  NInput::CT_UNKNOWN;
}

bool CWindow::IsPointInsideOfChildren( const CVec2 &vPoint )
{
	for ( int i = drawOrder.Size() - 1; i >= 0; --i )
	{
		if ( !drawOrder[i]->IsVisible() )
			continue;
		if ( drawOrder[i]->IsInside(vPoint) )
			return true;
	}
	return false;
}

bool CWindow::ProcessMessage( const struct SBUIMessage &msg )
{
	HM_TYPE::iterator it = handleMap.find( msg.szMessageID );
	bool bRes = false;
	
	NI_ASSERT( it != handleMap.end(), StrFmt( "window recieves unregistered message \"%s\"", msg.szMessageID.c_str() ) );
	if ( it != handleMap.end() )
	{
		bRes = it->second.Execute( msg, this );
	}

	if ( !bRes ) // this window doesn't process message, direct it to children
	{
		for ( int i = drawOrder.Size() - 1; i >= 0; --i )
			if ( drawOrder[i]->ProcessMessage( msg ) )
				return true;
	}
	return bRes;	
}


void CWindow::InitStatic()
{
	//REGISTER_MESSAGE_HANDLER(CWindow,ShowWindow,UI_SHOW_WINDOW)
	handleMap["UI_SHOW_WINDOW"] = CUIMessageHandler( CWindow::ShowWindow );
	handleMap["UI_SET_FOCUS"] = CUIMessageHandler( CWindow::SetFocus );
	handleMap["UI_ENABLE_WINDOW"] = CUIMessageHandler( CWindow::Enable );
}

void CWindow::CopyBackground( const IWindow *pSrcWnd )
{
	SetBackground( (dynamic_cast<const CWindow *>(pSrcWnd))->pBackground );
}

void CWindow::SetTexture( const struct NDb::STexture *_pDesc )
{
	NI_ASSERT( pBackground != 0, StrFmt( "Window \"%s\" has no background. Cannot change texture.", GetInstance()->szName ) );
	if ( pBackground != 0 )
		pBackground->SetTexture( _pDesc );
}

SWindowContext *CWindow::GetContext()
{
	if ( !pContext )
		pContext = new SWindowContextCommon();
	return pContext;
}

bool CWindow::MsgOnLMouseDown( const struct SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	if( !IsInside( ScreenToVirtual( vPos ) ) )
		return false;
	return OnButtonDown( vPos, MSTATE_BUTTON1 );
}

bool CWindow::MsgOnLMouseUp( const struct SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	return OnButtonUp( vPos, MSTATE_BUTTON1 );
}

bool CWindow::MsgOnLMouseDblClick( const struct SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	if( !IsInside( ScreenToVirtual( vPos ) ) )
		return false;
	return OnButtonDblClk( vPos, MSTATE_BUTTON1 );
}

bool CWindow::MsgOnRMouseDown( const struct SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	if( !IsInside( ScreenToVirtual( vPos ) ) )
		return false;
	return OnButtonDown( vPos, MSTATE_BUTTON2 );
}

bool CWindow::MsgOnRMouseUp( const struct SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	return OnButtonUp( vPos, MSTATE_BUTTON2 );
}

bool CWindow::MsgOnRMouseDblClick( const struct SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	if( !IsInside( ScreenToVirtual( vPos ) ) )
		return false;
	OnButtonDblClk( vPos, MSTATE_BUTTON2 );
	return true;
}

bool CWindow::MsgOnMouseMove( const struct SGameMessage &msg )
{
	NI_ASSERT( IsPacked2DCoords(msg.nParam1), "param is not a packed 2D coords!" );
	CVec2 vPos = UnPackCoords( msg.nParam1 );
	if( !IsInside( ScreenToVirtual( vPos ) ) )
		return false;
	return OnMouseMove( vPos, 0 );
}

void CWindow::SetFadeValue( float fValue )
{
	fFadeValue = Clamp( fValue, 0.0f, 1.0f );
}

void CWindow::SetInternalFadeValue( float fValue )
{
	fInternalFadeValue = fValue;
}

void CWindow::VisitText( struct IUIVisitor *pVisitor )
{
	if ( pPlacedText )
	{
		float fFade = GetTotalFadeValue();
		pPlacedText->SetInternalFadeValue( fFade );
		pPlacedText->Visit( pVisitor );
	}
}

UI_EXPORT void CheckInstantLoadTexture( const NDb::STexture *pTexture )
{
	if ( pTexture )
	{
		NI_ASSERT( pTexture->eType == NDb::STexture::TEXTURE_2D || pTexture->bInstantLoad, 
			StrFmt( "UI should use instant load of textures, dbid = \"%s\"", pTexture->GetDBID().ToString().c_str() ) );
	}
}

START_REGISTER(UI_Window)

REGISTER_VAR_EX( "check_duplicate_children", NGlobal::VarIntHandler, &CHECK_DUPLICATE_CHILDREN, 0, STORAGE_NONE );
REGISTER_VAR_EX( "check_ui_textures_instant_load", NGlobal::VarIntHandler, &CHECK_UI_TEXTURES_INSTANT_LOAD, 0, STORAGE_NONE );
REGISTER_VAR_EX( "ui_common_show_warnings", NGlobal::VarBoolHandler, &s_bUICommonShowWarnings, false, STORAGE_NONE );

FINISH_REGISTER


