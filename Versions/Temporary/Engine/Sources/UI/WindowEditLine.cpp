// WindowEditLine.cpp: implementation of the CWindowEditLine class.
//


#include "stdafx.h"
#include "3dmotor/rectlayout.h"
#include "WindowEditLine.h"

#include "UIVisitor.h"
#include "InterfaceConsts.h"
#include "UIML.h"
#include "UIScreen.h"


REGISTER_SAVELOAD_CLASS(0x11075B83,CWindowEditLine)


// CWindowEditLine


CWindowEditLine::CWindowEditLine() : timeSegment( 0 ), nCursorPos( 0 ), bShowCursor( 1 ), 
nBeginSel( -1 ), nEndSel( -1 ), m_nBeginDragSel( -1 ),
nBeginText( 0 ), bRegistered( false ), bMouseButton1Down( false )
{
	impotantMsgs.AddObserver( "editline_wordleft", &CWindowEditLine::OnCtrlLeft );
	impotantMsgs.AddObserver( "editline_wordright", &CWindowEditLine::OnCtrlRight );

	impotantMsgs.AddObserver( "editline_paste", &CWindowEditLine::OnPaste );
	impotantMsgs.AddObserver( "editline_copy", &CWindowEditLine::OnCopy );
	impotantMsgs.AddObserver( "editline_cut", &CWindowEditLine::OnCut );
	impotantMsgs.AddObserver( "editline_select_all", &CWindowEditLine::OnSelectAll );

	AddObserver( "win_char", &CWindowEditLine::OnChar );
	AddObserver( "win_key", &CWindowEditLine::OnKey );
}

void CWindowEditLine::RegisterObservers()
{
	CWindow::RegisterObservers();
	UpdateFocus();
}

void CWindowEditLine::UpdateFocus()
{
	SetFocus( IsFocused() );
}

bool CWindowEditLine::OnMouseMove( const CVec2 &_vPos, const int nButton )
{
	if ( CWindow::OnMouseMove( _vPos, nButton ) )
		return true;

	if ( bMouseButton1Down ) // selection is in progress
	{	
		nCursorPos = GetSelection( _vPos.x );
		if ( nBeginText + nCursorPos > m_nBeginDragSel )
		{
			nBeginSel = m_nBeginDragSel;
			nEndSel = nBeginText + nCursorPos;
		}
		else
		{
			nBeginSel = nBeginText + nCursorPos;
			nEndSel = m_nBeginDragSel;
		}
		EnsureCursorVisible();
		return true;
	}
	return false;
}

bool CWindowEditLine::OnButtonUp( const CVec2 &_vPos, const int nButton )
{
	if ( nButton & MSTATE_BUTTON1 )
	{
		bMouseButton1Down = false;
	}
	return CWindow::OnButtonUp( _vPos, nButton );
}

bool CWindowEditLine::OnButtonDown( const CVec2 &_vPos, const int nButton )
{
	if ( CWindow::OnButtonDown( _vPos, nButton ) )
		return true;

	if ( nButton & MSTATE_BUTTON1 )
	{
		if ( IsInside( _vPos ) )
		{
			if ( IsFocused() )
			{
				bMouseButton1Down = true;
				nCursorPos = GetSelection( _vPos.x );
				m_nBeginDragSel = nBeginSel = nEndSel = nCursorPos + nBeginText;
			}
			SetFocus( true );
			return true;
		}
		else
		{
			SetFocus( false );
		}
	}
	return false;
}

void CWindowEditLine::SetFocus( const bool bFocus )
{
	bool bNotify = false;
	if ( bFocus != IsFocused() )
		bNotify = true;

	if ( bFocus && !IsFocused() && !bMouseButton1Down )
	{
		nBeginSel = 0;
		nEndSel = wszFullText.size();
	}

	CWindow::SetFocus( bFocus );	

	if ( bNotify )
	{
		if ( pFocusNotify )
			pFocusNotify->OnFocus( bFocus );
		if ( !bFocus )
			RunAnimationAndCommands( pInstance->sequienceOnFocusLost, "", false, false );
	}
	
	if ( bFocus )
	{
		GetScreen()->RegisterToSegment( this, true );
	}
	else
	{
		m_nBeginDragSel = nBeginSel = nEndSel = -1;

		GetScreen()->RegisterToSegment( this, false );
	}
}

int CWindowEditLine::GetSelection( const int _nX )
{
	CTRect<float> editRect;
	FillWindowRectEditLine( &editRect );

	// rectsLayout - in screen coordinates :)
	CTPoint<float> vScreen( _nX - editRect.left, 0 );
	VirtualToScreen( vScreen, &vScreen );
	const int nX = vScreen.x;

	int nCur = 0, nPrev = 0;
	
	
	int i = 0;	// char number
	//nCur += editRect.left;
	for ( list<CTRect<float> >::const_iterator it = rectsList.begin(); it != rectsList.end(); ++it )
	{
		nCur += it->Width();
		if ( nCur > nX )
		{
			if ( nX - nPrev < nCur - nX && i > 0 )			//ближе к левой букве чем к правой
				i--;
			break;
		}
		nPrev = nCur;
		++i;
	}
	

	//if ( nCur <= nX && i > 0 )		//нажато правее края текста
		//i--;
	NI_ASSERT( i >= 0 && i <= wszFullText.size(), "Error in CWindowEditLine::GetSelection()" );
	return i;
}

void CWindowEditLine::SetCursor( const int nPos )
{
	if ( nPos < 0 )
	{
		//nCursorPos = wszFullText.length();
		nCursorPos = rectsList.size();
	}
	else
		nCursorPos = nPos; 
}

bool CWindowEditLine::DeleteSelection()
{
	if ( nEndSel == nBeginSel )
		return false;

	if ( nBeginSel != -1 )
	{
		if ( nEndSel < 0 || nEndSel > wszFullText.size() )
			nEndSel = wszFullText.size();
		if ( nBeginSel > nEndSel )
			nBeginSel = nEndSel;
		wszFullText.erase( nBeginSel, nEndSel - nBeginSel );
		nCursorPos = nBeginSel - nBeginText;
		nBeginSel = nEndSel = -1;
		return true;
	}
	return false;
}

bool CWindowEditLine::IsValidSymbol( const wchar_t chr )const
{
	static const wstring szInValidSymbols = L"<>";
	static const int nLen = szInValidSymbols.size();
	for ( int i = 0; i < nLen; i++ )
	{
		if ( chr == szInValidSymbols[i] )
			return false;
	}

	switch( pInstance->eTextEntryType )
	{
	case NDb::ETET_GAME_SPY:
		{
			//проверим, что символ удовлетворяет требованиям GameSpy NickName
			static const wstring szValidSymbols = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789[]\\`_^{|}-";
			static const int nLen = szValidSymbols.size();
			for ( int i = 0; i < nLen; i++ )
			{
				if ( chr == szValidSymbols[i] )
					return true;
			}
			return false;
		}
		break;
	case NDb::ETET_FILENAME:
		{
			//проверим, что символ удовлетворяет требованиям FileName symbols
			static const wstring szValidSymbols = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789[]`_^{}-!@#$%^&()+=~";
			static const int nLen = szValidSymbols.size();
			for ( int i = 0; i < nLen; i++ )
			{
				if ( chr == szValidSymbols[i] )
					return true;
			}
			return false;
		}
		break;
	case NDb::ETET_NUMERIC:
		{
			static const wstring szValidSymbols = L"0123456789";
			static const int nLen = szValidSymbols.size();
			for ( int i = 0; i < nLen; i++ )
			{
				if ( chr == szValidSymbols[i] )
					return true;
			}
			return false;
		}
		break;
	case NDb::ETET_LOCAL_PLAYERNAME:
		{
			static const wstring szInValidSymbols = L"&'\"<>";
			static const int nLen = szInValidSymbols.size();
			for ( int i = 0; i < nLen; i++ )
			{
				if ( chr == szInValidSymbols[i] )
					return false;
			}
		}
		break;
	case NDb::ETET_ALL:
		break;
	default:
		NI_ASSERT( false, StrFmt( "wrong edit line type %i", pInstance->eTextEntryType ) );
		return false;
	}
	return true;
}

bool CWindowEditLine::OnReturn()
{
	if ( !IsFocused() )
		return false;

	return RunAnimationAndCommands( pInstance->sequienceOnReturn, pInstance->szOnReturn, false, false );
}

void CWindowEditLine::OnPaste( const struct SGameMessage &msg )
{
	if ( !IsFocused() )
		return;

	if ( !IsClipboardFormatAvailable(CF_TEXT) ) 
		return; 
	if ( !OpenClipboard( 0 ) ) 
		return; 

	HANDLE h = GetClipboardData( CF_TEXT );
	if ( NULL != h )
	{
		LPTSTR lptstr = (LPTSTR)GlobalLock(h); 
		if ( lptstr != NULL ) 
		{ 
			const wstring szInserted( NStr::ToUnicode(lptstr) );
			GlobalUnlock(h); 
			CloseClipboard();

			const wstring wszOldText = wszFullText;
			const int nOldCursorPos = nCursorPos;
			const int nStrLen = szInserted.size();
			for ( int i = 0; i < nStrLen; ++i )
			{
				if ( !IsValidSymbol( szInserted[i] ) )
					return;
			}
			
			DeleteSelection();
			wszFullText.insert( Max(0,nBeginText+nCursorPos), szInserted.c_str() );

			nCursorPos+= szInserted.size();
			if ( !CheckTextInsideEditLine() )
			{
				wszFullText = wszOldText;
				nCursorPos = nOldCursorPos;
				EnsureCursorVisible();
				return ;
			}
			EnsureCursorVisible();
		}
		else
			GlobalUnlock(h); 
		CloseClipboard();
	}
	else
		CloseClipboard();

}

void CWindowEditLine::OnCopy( const struct SGameMessage &msg )
{
	if ( !IsFocused() )
		return;

	CopySelectionToClipboard();
}

void CWindowEditLine::OnCut( const struct SGameMessage &msg )
{
	if ( !IsFocused() )
		return;

	CopySelectionToClipboard();

	DeleteSelection();
	EnsureCursorVisible();
}

void CWindowEditLine::OnSelectAll( const struct SGameMessage &msg )
{
	if ( !IsFocused() )
		return;

	nBeginSel = 0;
	nEndSel = wszFullText.size();
}

void CWindowEditLine::CopySelectionToClipboard()
{
	if ( pInstance->bPassword || !OpenClipboard( 0 ) ) 
		return; 

	// If text is selected
	if ( nBeginSel == nEndSel )
	{
		CloseClipboard();
		return;
	}
	EmptyClipboard(); 

	// Allocate a global memory object for the text. 
	HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (nEndSel - nBeginSel + 1) * sizeof(char)); 
	if ( hglbCopy == NULL ) 
	{ 
		CloseClipboard(); 
		return; 
	} 

	// get selection
	wstring wszToCopy = &wszFullText[nBeginSel];
	wszToCopy.resize( nEndSel - nBeginSel );
	const string szToCopy( NStr::ToMBCS( wszToCopy ) );

	// Lock the handle and copy the text to the buffer. 
	LPTSTR lptstrCopy = (LPTSTR)GlobalLock( hglbCopy ); 

	memcpy( lptstrCopy, szToCopy.c_str(), (szToCopy.size()) * sizeof(char) ); 
	lptstrCopy[szToCopy.size()] = (char) 0;    // null character 
	GlobalUnlock( hglbCopy ); 

	SetClipboardData( CF_TEXT, hglbCopy ); 
	CloseClipboard(); 
}

void CWindowEditLine::OnTab( const struct SGameMessage &msg )
{
	if ( !IsFocused() )
		return;

	const wstring wszOldText = wszFullText;
	const int nOldCursorPos = nCursorPos;

	if ( pInstance->eTextEntryType != NDb::ETET_NUMERIC && pInstance->eTextEntryType != NDb::ETET_GAME_SPY && pInstance->eTextEntryType != NDb::ETET_FILENAME )
	{
		DeleteSelection();
		wszFullText.insert( wszFullText.begin() + nBeginText + nCursorPos, VK_SPACE );
		wszFullText.insert( wszFullText.begin() + nBeginText + nCursorPos, VK_SPACE );
		wszFullText.insert( wszFullText.begin() + nBeginText + nCursorPos, VK_SPACE );
		wszFullText.insert( wszFullText.begin() + nBeginText + nCursorPos, VK_SPACE );
		nCursorPos += 4;
		if ( !CheckTextInsideEditLine() )
		{
			wszFullText = wszOldText;
			nCursorPos = nOldCursorPos;
		}
		EnsureCursorVisible();
	}
}

void CWindowEditLine::OnBack( const struct SGameMessage &msg )
{
	if ( !IsFocused() )
		return;

	if ( !DeleteSelection() && nBeginText + nCursorPos > 0 )
	{
		wszFullText.erase( nBeginText + nCursorPos - 1, 1 );
		nCursorPos--;
	}
	CheckTextInsideEditLine();
	EnsureCursorVisible();
}

void CWindowEditLine::OnDelete( const struct SGameMessage &msg )
{
	if ( !IsFocused() )
		return;

	if ( !DeleteSelection() && nBeginText + nCursorPos < wszFullText.size() )
	{
		wszFullText.erase( nBeginText+nCursorPos, 1 );
	}
	CheckTextInsideEditLine();
	EnsureCursorVisible();
}

void CWindowEditLine::OnLeft( const struct SGameMessage &msg )
{
	if ( !IsFocused() )
		return;

	nBeginSel = nEndSel = -1;
	if ( nBeginText+nCursorPos == 0 )
		return;
	//на одну позицию влево
	nCursorPos--;
	EnsureCursorVisible();
}

void CWindowEditLine::OnCtrlLeft( const struct SGameMessage &msg )
{
	if ( !IsFocused() )
		return;

	nBeginSel = nEndSel = -1;
	if ( nBeginText+nCursorPos == 0 )
		return;

	//Если нажата crtl и стрелка влево, то сдвигаемся влево на одно слово
	while( nBeginText+nCursorPos > 0 && isspace(wszFullText[nBeginText+nCursorPos-1]) )
		nCursorPos--;
	if ( nBeginText+nCursorPos > 0 )
	{
		if ( isalpha(wszFullText[nBeginText+nCursorPos-1]) )
			while( nBeginText+nCursorPos > 0 && isalpha(wszFullText[nBeginText+nCursorPos-1]) )
				nCursorPos--;
			else
				while( nBeginText+nCursorPos > 0 && !isalpha(wszFullText[nBeginText+nCursorPos-1]) )
					nCursorPos--;
	}
	EnsureCursorVisible();
}

void CWindowEditLine::OnRight( const struct SGameMessage &msg )
{
	if ( !IsFocused() )
		return;

	nBeginSel = nEndSel = -1;
	if ( nBeginText+nCursorPos == wszFullText.size() )
		return;
	//на одну позицию вправо
	nCursorPos++;
	EnsureCursorVisible();
}

void CWindowEditLine::OnCtrlRight( const struct SGameMessage &msg )
{
	if ( !IsFocused() )
		return;

	//Если нажата crtl и стрелка вправо, то сдвигаемся вправо на одно слово
	if ( nBeginText+nCursorPos < wszFullText.size() )
	{
		if ( isalpha(wszFullText[nBeginText+nCursorPos]) )
			while( nBeginText+nCursorPos < wszFullText.size() && isalpha(wszFullText[nBeginText+nCursorPos]) )
				nCursorPos++;
			else
				while( nBeginText+nCursorPos < wszFullText.size() && !isalpha(wszFullText[nBeginText+nCursorPos]) )
					nCursorPos++;
	}
	
	while( nBeginText+nCursorPos < wszFullText.size() && isspace(wszFullText[nBeginText+nCursorPos]) )
		nCursorPos++;
	EnsureCursorVisible();
}

void CWindowEditLine::OnHome( const struct SGameMessage &msg )
{
	if ( !IsFocused() )
		return;
	nBeginSel = nEndSel = -1;
	//на начало строки
	nBeginText = 0;
	nCursorPos = 0;
	EnsureCursorVisible();
}

void CWindowEditLine::OnEnd( const struct SGameMessage &msg )
{
	if ( !IsFocused() )
		return;

	nBeginSel = nEndSel = -1;
	//на конец строки
	nCursorPos = wszFullText.size() - nBeginText;
	EnsureCursorVisible();
}

bool CWindowEditLine::OnEscape()
{
	if ( !IsFocused() )
		return false;

	nBeginSel = nEndSel = -1;
	return RunAnimationAndCommands( pInstance->sequienceOnEscape, pInstance->szOnEscape, false, true );
}

bool CWindowEditLine::OnChar( const SGameMessage &msg )
{
	if ( !IsFocused() || bMouseButton1Down )
		return false;

	const wchar_t chr = msg.nParam1;//.wCharCode;

	if ( !iswprint( msg.nParam1 ) )
		return false;
	if ( msg.nParam1 < 32 )
		return false;

	//Если печатный символ, то просто выводим его
	AddChar( chr );
	NotifyTextChanged();
	return false;
}

bool CWindowEditLine::OnKey( const SGameMessage &msg )
{
	if ( !IsFocused() )
		return false;

	switch( msg.nParam1 )
	{
	case VK_END: OnEnd( msg ); return true;
	case VK_HOME: OnHome( msg ); return true;
	case VK_LEFT: OnLeft( msg ); return true;
	case VK_RIGHT: OnRight( msg ); return true;
	case VK_BACK: OnBack( msg ); return true;
	case VK_DELETE: OnDelete( msg ); return true;
//	case VK_RETURN: OnReturn( msg ); return true;
//	case VK_ESCAPE: OnEscape( msg ); return false;
	//case VK_TAB: OnTab( msg ); return true;
	}
	return false;
}

bool CWindowEditLine::AddChar( const wchar_t chr )
{
	const wstring wszOldText = wszFullText;
	const int nOldCursorPos = nCursorPos;
	if ( IsValidSymbol( chr ) )
	{
		DeleteSelection();
		wszFullText.insert( wszFullText.begin() + Min( int(wszFullText.size()), nBeginText + nCursorPos), chr );
		nCursorPos++;
		if ( !CheckTextInsideEditLine() )
		{
			wszFullText = wszOldText;
			nCursorPos = nOldCursorPos;
			EnsureCursorVisible();
			return false;
		}
		EnsureCursorVisible();
	}
	return true;
}

void CWindowEditLine::Init()
{
	CWindow::Init();
	InitLocal();
	
	UpdateFocus();
	static int nTabOrder = 0;
	if ( pInstance->nTabOrder == -1 )
	{
		checked_cast<CWindowScreen*>(GetScreen())->RegisterTabOrder( this, nTabOrder );
		++nTabOrder;
	}
	else
		checked_cast<CWindowScreen*>(GetScreen())->RegisterTabOrder( this, pInstance->nTabOrder );
}

void CWindowEditLine::InitLocal()
{
	CreateText();
}

void CWindowEditLine::CreateText()
{
	if ( !pGfxText )
	{
		pGfxText = CreateML();
		CUIFactory::RegisterMLHandlers( pGfxText );
		SetTextToGfx( L"" );
		CTRect<float> rc;
		FillWindowRectEditLine( &rc );
		pGfxText->Generate( VirtualToScreenX( rc.GetSizeX() ) );
	}
}

int CWindowEditLine::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CWindow*>( this ) );
	saver.Add( 2, &pGfxText );
	saver.Add( 3, &timeSegment );							// for counting segment times
	saver.Add( 4, &nCursorPos );									//позиция курсора в текущей редактируемой строке
	saver.Add( 5, &bShowCursor );								//для мигания курсора
	saver.Add( 7, &nBeginSel );								//начало выделения
	saver.Add( 8, &nEndSel );									//конец выделения
	saver.Add( 10, &m_nBeginDragSel );						//начало выделения мышкой
	saver.Add( 16, &nBeginText );		//с этой позиции начинается отображение текста szFullText
	saver.Add( 18, &wszFullText );
	saver.Add( 27, &bRegistered );												// message sink registered
	saver.Add( 28, &pShared );
	saver.Add( 29, &pInstance );
	saver.Add( 30, &bRegistered );
	if ( saver.IsReading() )
		bMouseButton1Down = false;
	return 0;
}

void CWindowEditLine::AfterLoad()
{
	InitLocal();
	CWindow::AfterLoad();
}

void CWindowEditLine::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	const NDb::SWindowEditLine *pDesc( checked_cast<const NDb::SWindowEditLine*>( _pDesc ) );
	pInstance = pDesc->Duplicate();

	CWindow::InitByDesc( _pDesc );
	pShared = checked_cast_ptr<const NDb::SWindowEditLineShared *>( pDesc->pShared );
}

void CWindowEditLine::Reposition( const CTRect<float> &rcParent )
{
	CWindow::Reposition( rcParent );
	int nW;
	GetPlacement( 0, 0, &nW, 0 );
	CreateText();
	EnsureCursorVisible();
	pGfxText->Generate( VirtualToScreenX( nW ) );
}

int CWindowEditLine::GetTextWidth( const int nFirstChars )
{
	if ( nFirstChars < 0 )
	{
		if ( rectsList.empty() )
			return 0;
		list<CTRect<float> >::const_iterator pos = rectsList.end();
		--pos;
		return pos->x2;
	}
	list<CTRect<float> >::const_iterator it = rectsList.begin();
	
	advance( it, nFirstChars );
	if ( it == rectsList.end() )
		return GetTextWidth( -1 );
	else
		return it->x1;
}

void CWindowEditLine::Visit( struct IUIVisitor *pVisitor )
{
	CWindow::Visit( pVisitor );

	if ( !IsVisible() )
		return;

	CTRect<float> wndRect;
	FillWindowRectEditLine( &wndRect );
	CTRect<float> textRect( wndRect );

	int nH = pGfxText->GetSize().y;
	if ( nH == 0 )
	{
		pGfxText->SetText( L"A", 0 );
		pGfxText->Generate( 1024 );
		nH = pGfxText->GetSize().y;
		pGfxText->SetText( L"", 0 );
		pGfxText->Generate( 1024 );
	}
	VirtualToScreen( textRect, &textRect );
	VirtualToScreen( wndRect, &wndRect );
	textRect.top = wndRect.top + ( wndRect.Height() - nH ) / 2;
	textRect.bottom = textRect.top + nH;


	// рисуем выделение
	if ( nBeginSel != -1 && nBeginSel != nEndSel )
	{
		CRectLayout rectLayout;
		CTRect<float> rect;
		int nBegin = 0;
		if ( nBeginSel - nBeginText >= -1 )
			nBegin = GetTextWidth( nBeginSel - nBeginText );
		int nEnd = GetTextWidth( nEndSel - nBeginText );
		
		rect.left = wndRect.left + nBegin;
		rect.right = wndRect.left + nEnd;
		if ( rect.right > wndRect.right - 1 )
			rect.right = wndRect.right - 1;
		rect.top = textRect.top;
		rect.bottom = textRect.bottom;
		
		rectLayout.AddRect( rect.x1, rect.y1, rect.Width(), rect.Height(), CTRect<float>(0,0,0,0), NGfx::SPixel8888( pShared->nSelColor ) );
		pVisitor->VisitUIRect( 0, 0, rectLayout );
	}
	
	// рисуем текст
	pVisitor->VisitUIText( pGfxText, textRect.GetLeftTop(), textRect );

	// рисуем курсор
	if ( IsFocused() && bShowCursor )
	{
		CRectLayout rectLayout;
		int nPos = GetTextWidth( nCursorPos );
		rectLayout.AddRect( textRect.x1 + nPos, textRect.y1 , 2, textRect.Height(), CTRect<float>(0,0,0,0), NGfx::SPixel8888( pShared->nCursorColor ) );
		pVisitor->VisitUIRect( 0, 0, rectLayout );
	}
}

void CWindowEditLine::Segment( const int timeDiff )
{
	timeSegment += timeDiff;
	if ( timeSegment > CInterfaceConsts::CURSOR_ANIMATION_TIME() )
	{
		timeSegment = 0;
		bShowCursor = !bShowCursor;
	}
}

void CWindowEditLine::NotifyTextChanged()
{
	if ( !IsFocused() )
		return;

	RunAnimationAndCommands( pInstance->sequienceOnTextChanged, pInstance->szOnTextChanged, false, false );

	/*
	SUIMessage msg;
	msg.nMessageCode = UI_NOTIFY_EDIT_BOX_TEXT_CHANGED;
	msg.nFirst = GetWindowID();
	msg.nSecond = 0;
	GetParent()->ProcessMessage( msg );
	*/
}

void CWindowEditLine::SetText( const wchar_t *pszText )
{
	wszFullText = pszText;
	SetTextToGfx( wszFullText );
	nBeginText = 0;
	nCursorPos = rectsList.size();
	if ( IsVisible() )
		EnsureCursorVisible();
	m_nBeginDragSel = nBeginSel = nEndSel = -1;
}

void CWindowEditLine::EnsureCursorVisible()
{
	bShowCursor = true;
	CTRect<float> wndRect;
	FillWindowRectEditLine( &wndRect );
	
	/*IText *pText = pGfxText->GetText();
	pText->SetText( wszFullText.c_str() + nBeginText );
	pGfxText->SetText( pText );
*/
	wstring wszTmp = wszFullText.c_str() + nBeginText;
	SetTextToGfx( wszTmp );

	if ( nCursorPos <= 0 && nBeginText > 0 )
	{
		NI_ASSERT( pInstance->bTextScroll, "Edit box error: nCursorPos < 0 and pInstance->bTextScroll == true" );
		//курсор левее левого края edit box, подвинем текст вправо, чтобы курсор стал видимым
		nBeginText += nCursorPos;
		nCursorPos = 0;
		if ( nBeginText < 0 )
			nBeginText = 0;
		if ( nBeginText > 0 )
		{
			nBeginText--;
			nCursorPos++;
		}
		if ( nBeginText > 0 )
		{
			nBeginText--;
			nCursorPos++;
		}
		const wstring wszTmp = wszFullText.c_str() + nBeginText;
		SetTextToGfx( wszTmp );
	}
	else if ( GetTextWidth( nCursorPos ) > wndRect.Width() - 2 )
	{
		//курсор правее правого края edit box, подвинем текст влево, чтобы курсор стал видимым
		while ( GetTextWidth( nCursorPos ) > wndRect.Width() - 2 )		//2 is the width of cursor
		{
			if ( pInstance->bTextScroll )
			{
				nBeginText++;
				nCursorPos--;
			}
			else if ( !wszFullText.empty() )
			{
				wszFullText.erase( wszFullText.size() - 1 );
			}
			if ( !wszFullText.empty() )
			{
				const wstring wszTmp = wszFullText.c_str() + nBeginText;
				SetTextToGfx( wszTmp );
			}
			else
			{
				SetTextToGfx( wszFullText );
				break;
			}
		}
	}
}

void CWindowEditLine::SetTextToGfx( const wstring &szText )
{
	CTRect<float> wndRect;
	FillWindowRectEditLine( &wndRect );
	wstring wszText;
	if ( pInstance->bPassword )
	{
		for ( int i = szText.size(); i != 0 ; i-- )
			wszText += L"*";
	}
	else
	{
		wszText = szText;
	}

	pGfxText->SetText( wszText, 0 );
	pGfxText->Generate( VirtualToScreenX( wndRect.Width() ) );
	rectsList.clear();
	pGfxText->Render( &rectsList, CTPoint<float>( 0, 0 ), CTRect<float>( 0, 0, 0, 0 ) );
	//DEBUG{
	list<CTRect<float> >::const_iterator it = rectsList.begin();
	while ( it != rectsList.end() )
	{
		CTRect<float> dbg = *it;
		++it;
		//return it->x1;
	}
	//DEBUG}
}

bool CWindowEditLine::CheckTextInsideEditLine()
{
	CTRect<float> wndRect;
	FillWindowRectEditLine( &wndRect );

	wstring wszTmp = wszFullText.c_str() + nBeginText;
	SetTextToGfx( wszTmp );

	//сперва проверим ограничение на максимальную длину текста
	if ( pInstance->nMaxLength != -1 && rectsList.size() > pInstance->nMaxLength )
		return false;

	if ( pInstance->bTextScroll )
		return true;

	int nTextWidth = 0;
	for ( list<CTRect<float> >::const_iterator it = rectsList.begin(); it != rectsList.end(); ++it )
		nTextWidth += it->Width();
	return GetTextWidth( -1 ) < wndRect.Width() - 2;
}

void CWindowEditLine::SetSelection( const int nBegin, const int nEnd )
{
	nBeginSel = nBegin; 
	nEndSel = nEnd;
}

void CWindowEditLine::FillWindowRectEditLine( CTRect<float> *pRect )
{
	FillWindowRect( pRect );
	pRect->left += pShared->nLeftSpace;
	pRect->right -= pShared->nRightSpace;
	pRect->top += pShared->nYOffset;
}

bool CWindowEditLine::ProcessEvent( const struct SGameMessage &msg )
{
	NInput::CBind bindEnter("enter_pressed");
	if ( bindEnter.ProcessEvent( msg ) )
	{
		if ( OnReturn() )
			return true;
	}
	NInput::CBind bindEsc("esc_pressed");
	if ( bindEsc.ProcessEvent( msg ) )
	{
		if ( OnEscape() )
			return true;
	}

	impotantMsgs.ProcessEvent( msg, this );

	if (NInput::IsDInputDiscardableKey( msg.mMessage ) )
		return IsFocused();
	else 
		return CWindow::ProcessEvent( msg );
}

