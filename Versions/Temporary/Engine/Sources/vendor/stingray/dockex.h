#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxpriv.h>

#include "Toolkit/sbarcore.h"

// DockControlBarEx, shared by the two frame classes that publish it.
//
// SECFrameWnd and SECMDIFrameWnd are unrelated types -- one derives from
// CFrameWnd and the other from CMDIFrameWnd -- and each declares its own
// DockControlBarEx. Only the MDI one is ever reached from this editor, which is
// worth knowing: an implementation put on the other has no caller and looks,
// from a running editor, exactly like no implementation at all. Both are
// CFrameWnd in the end, so the work lives here once and each forwards to it.
namespace NDockEx
{

// Where row nRow of a dock bar ends, as an index into m_arrBars, or -1 when the
// dock bar has no such row.
//
// A CDockBar keeps its bars in one flat array with a null marking the end of
// each row, and it starts life holding a single null -- one empty row. Empty
// rows are not counted here: MFC lays them out as nothing, so counting them
// would let a bar be "joined" to a row with nothing in it, which is just the
// bar on its own again.
inline int FindRowEnd( const CPtrArray &arrBars, int nRow )
{
	int nRowIndex = 0;
	int nBarsInRow = 0;
	for ( int i = 0; i < arrBars.GetSize(); ++i )
	{
		if ( arrBars[i] != nullptr )
		{
			++nBarsInRow;
			continue;
		}
		if ( nBarsInRow > 0 )
		{
			if ( nRowIndex == nRow )
			{
				return i;
			}
			++nRowIndex;
			nBarsInRow = 0;
		}
	}
	return -1;
}

// Move a bar that was just docked into a row of its own into row nRow instead.
//
// Done by reordering the dock bar's array rather than by handing
// CDockBar::DockControlBar a rectangle to aim at, which is the documented way
// and does not work here: CDockBar::Insert picks a row by comparing the
// rectangle against the bars already docked, and skips any bar that is not
// visible. At CMainFrame::OnCreate time the frame itself is not on screen yet,
// so every bar already docked is invisible, none of them is a candidate, and
// every bar lands in a new row however it is asked. Reordering needs no
// geometry and no visibility, and it runs after DockControlBar has done all of
// its own bookkeeping.
inline void MoveBarToRow( CDockBar *pDockBar, CControlBar *pBar, int nRow )
{
	if ( pDockBar == nullptr || pBar == nullptr || nRow < 0 )
	{
		return;
	}
	CPtrArray &arrBars = pDockBar->m_arrBars;
	int nBarPos = -1;
	for ( int i = 0; i < arrBars.GetSize(); ++i )
	{
		if ( arrBars[i] == pBar )
		{
			nBarPos = i;
			break;
		}
	}
	if ( nBarPos < 0 )
	{
		return;
	}
	// Only a bar sitting alone in its own row can be moved out of it without
	// disturbing anything else, which is exactly what DockControlBar has just
	// made: it appends the bar and then a null to close the row behind it.
	const bool bAloneInRow = ( nBarPos + 1 < arrBars.GetSize() )
		&& ( arrBars[nBarPos + 1] == nullptr )
		&& ( nBarPos == 0 || arrBars[nBarPos - 1] == nullptr );
	if ( !bAloneInRow )
	{
		return;
	}
	arrBars.RemoveAt( nBarPos + 1 );
	arrBars.RemoveAt( nBarPos );

	const int nInsertAt = FindRowEnd( arrBars, nRow );
	if ( nInsertAt < 0 )
	{
		// No row of that number to join, so put it back in one of its own.
		arrBars.Add( pBar );
		arrBars.Add( nullptr );
		return;
	}
	arrBars.InsertAt( nInsertAt, pBar );
}

// Dock a bar where the caller asked, at the size the caller asked for.
//
// CFrameWnd::DockControlBar takes a bar and a dock bar and nothing else, and
// gives every bar it is handed a row of its own at the end of that dock bar.
// That is why the editor's docking windows came up side by side rather than as
// MainFrame_Consts asks for them: the database browser and the property browser
// are both meant to share one column on the left, half its height each, and
// instead they took a column each and 530 pixels of the frame with them.
//
// The two sizing numbers go to the bar, because a docked bar is the only thing
// that is ever asked how big it is -- see SECControlBar::CalcDynamicLayout.
// nHeight is the thickness across the docked edge, fPctWidth the share of that
// edge's length.
//
// nRow picks which of the dock bar's rows to join, counting from the first one
// that has anything in it, and a row past the end gets a row of its own. nCol
// is a position within the row; every call site in this editor passes 0 for
// both, so appending to an existing first row is the only path exercised, and
// nCol is not acted on at all.
inline void DockControlBarEx( CFrameWnd *pFrame, CControlBar *pBar, UINT nDockBarID,
                              int nCol, int nRow, float fPctWidth, int nHeight )
{
	if ( pFrame == nullptr || pBar == nullptr || pBar->GetSafeHwnd() == nullptr )
	{
		return;
	}
	if ( SECControlBar *const pSecBar = dynamic_cast< SECControlBar * >( pBar ) )
	{
		pSecBar->SetDockedLayout( nHeight, fPctWidth );
	}
	else if ( nHeight > 0 )
	{
		// Not one of ours, so it has no share to keep, but the thickness is a
		// plain CControlBar member and still means something.
		pBar->m_nMRUWidth = static_cast< UINT >( nHeight );
	}

	pFrame->DockControlBar( pBar, nDockBarID );
	MoveBarToRow( pBar->m_pDockBar, pBar, nRow );
}

}
