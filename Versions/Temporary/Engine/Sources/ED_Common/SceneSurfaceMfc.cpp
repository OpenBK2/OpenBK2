#include "stdafx.h"
#include "MapEditorLib/ToolkitChoice.h"

#include "MapEditorLib/MfcPaintContext.h"
#include "MapEditorLib/MfcWidget.h"
#include "ChildFrameWndBase.h"
#include "SceneSurface.h"

#include <cstdlib>

// The viewport's window as it has always been: a CWnd made as the document
// window's AFX_IDW_PANE_FIRST, so the MDI child lays it out and routes commands
// to it. Every handler does what CChildFrameWndBase's did with its message,
// the CWnd base call first, and then hands the message to the core.

namespace
{
	class CMfcSceneSurface : public CWnd, public ISceneSurface, public IWidget
	{
		static const UINT_PTR UPDATE_SCENE_TIMER_ID = 1;

		CChildFrameWndBase *pCore = nullptr;
		UINT_PTR nUpdateSceneTimer = 0;

	protected:
		virtual BOOL PreCreateWindow( CREATESTRUCT &rCreateStruct )
		{
			if ( !CWnd::PreCreateWindow( rCreateStruct ) )
			{
				return FALSE;
			}

			rCreateStruct.dwExStyle |= WS_EX_CLIENTEDGE;
			rCreateStruct.style |= pCore->IsScrollEnabled() ? ( WS_VSCROLL | WS_HSCROLL ) : 0;
			rCreateStruct.style &= ~WS_BORDER;
			rCreateStruct.lpszClass = AfxRegisterWndClass( CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
																										 ::LoadCursor( NULL, IDC_ARROW ),
																										 HBRUSH( COLOR_WINDOW + 1 ),
																										 NULL );

			return TRUE;
		}

		afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct )
		{
			if ( CWnd::OnCreate( pCreateStruct ) == -1 )
			{
				return -1;
			}
			return pCore->OnCreate( this ) ? 0 : -1;
		}

		afx_msg void OnDestroy()
		{
			pCore->OnDestroy();
			CWnd::OnDestroy();
		}

		afx_msg void OnTimer( UINT_PTR nIDEvent )
		{
			if ( nIDEvent == UPDATE_SCENE_TIMER_ID )
			{
				pCore->OnTimer();
			}
			CWnd::OnTimer( nIDEvent );
		}

		afx_msg void OnSetFocus( CWnd* pOldWnd )
		{
			CWnd::OnSetFocus( pOldWnd );
			CWndWidget oldWidget( pOldWnd );
			pCore->OnSetFocus( &oldWidget );
		}

		afx_msg void OnKillFocus( CWnd* pNewWnd )
		{
			CWnd::OnKillFocus( pNewWnd );
			CWndWidget newWidget( pNewWnd );
			pCore->OnKillFocus( &newWidget );
		}

		afx_msg void OnMouseMove( unsigned nFlags, CPoint point )
		{
			CWnd::OnMouseMove( nFlags, point );
			pCore->OnMouseMove( nFlags, CTPoint<int>( point.x, point.y ) );
		}

		afx_msg BOOL OnMouseWheel( unsigned nFlags, short zDelta, CPoint point )
		{
			const BOOL bResult = CWnd::OnMouseWheel( nFlags, zDelta, point );
			return pCore->OnMouseWheel( nFlags, zDelta, CTPoint<int>( point.x, point.y ), bResult != FALSE ) ? TRUE : FALSE;
		}

		afx_msg void OnLButtonDown( unsigned nFlags, CPoint point )
		{
			CWnd::OnLButtonDown( nFlags, point );
			pCore->OnLButtonDown( nFlags, CTPoint<int>( point.x, point.y ) );
		}

		afx_msg void OnLButtonUp( unsigned nFlags, CPoint point )
		{
			CWnd::OnLButtonUp( nFlags, point );
			pCore->OnLButtonUp( nFlags, CTPoint<int>( point.x, point.y ) );
		}

		afx_msg void OnLButtonDblClk( unsigned nFlags, CPoint point )
		{
			CWnd::OnLButtonDblClk( nFlags, point );
			pCore->OnLButtonDblClk( nFlags, CTPoint<int>( point.x, point.y ) );
		}

		afx_msg void OnRButtonDown( unsigned nFlags, CPoint point )
		{
			CWnd::OnRButtonDown( nFlags, point );
			pCore->OnRButtonDown( nFlags, CTPoint<int>( point.x, point.y ) );
		}

		afx_msg void OnRButtonUp( unsigned nFlags, CPoint point )
		{
			CWnd::OnRButtonUp( nFlags, point );
			pCore->OnRButtonUp( nFlags, CTPoint<int>( point.x, point.y ) );
		}

		afx_msg void OnRButtonDblClk( unsigned nFlags, CPoint point )
		{
			CWnd::OnRButtonDblClk( nFlags, point );
			pCore->OnRButtonDblClk( nFlags, CTPoint<int>( point.x, point.y ) );
		}

		afx_msg void OnMButtonDown( unsigned nFlags, CPoint point )
		{
			CWnd::OnMButtonDown( nFlags, point );
			pCore->OnMButtonDown( nFlags, CTPoint<int>( point.x, point.y ) );
		}

		afx_msg void OnMButtonUp( unsigned nFlags, CPoint point )
		{
			CWnd::OnMButtonUp( nFlags, point );
			pCore->OnMButtonUp( nFlags, CTPoint<int>( point.x, point.y ) );
		}

		afx_msg void OnMButtonDblClk( unsigned nFlags, CPoint point )
		{
			CWnd::OnMButtonDblClk( nFlags, point );
			pCore->OnMButtonDblClk( nFlags, CTPoint<int>( point.x, point.y ) );
		}

		afx_msg void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
		{
			CWnd::OnKeyDown( nChar, nRepCnt, nFlags );
			pCore->OnKeyDown( nChar, nRepCnt, nFlags );
		}

		afx_msg void OnKeyUp( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
		{
			CWnd::OnKeyUp( nChar, nRepCnt, nFlags );
			pCore->OnKeyUp( nChar, nRepCnt, nFlags );
		}

		afx_msg void OnChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
		{
			CWnd::OnChar( nChar, nRepCnt, nFlags );
			pCore->OnChar( nChar, nRepCnt, nFlags );
		}

		afx_msg void OnSysKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
		{
			CWnd::OnSysKeyDown( nChar, nRepCnt, nFlags );
			pCore->OnSysKeyDown( nChar, nRepCnt, nFlags );
		}

		afx_msg void OnSysKeyUp( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
		{
			CWnd::OnSysKeyUp( nChar, nRepCnt, nFlags );
			pCore->OnSysKeyUp( nChar, nRepCnt, nFlags );
		}

		afx_msg void OnSysChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
		{
			CWnd::OnSysChar( nChar, nRepCnt, nFlags );
			pCore->OnSysChar( nChar, nRepCnt, nFlags );
		}

		afx_msg void OnContextMenu( CWnd *pWnd, CPoint point )
		{
			CWnd::OnContextMenu( pWnd, point );
			pCore->OnContextMenu( CTPoint<int>( point.x, point.y ) );
		}

		afx_msg BOOL OnEraseBkgnd( CDC* pDC )
		{
			return FALSE;
		}

		afx_msg void OnPaint()
		{
			if ( !pCore->BeginPaint() )
			{
				// A nested paint while the window moves itself: validated, and
				// nothing more.
				CPaintDC dc( this );
				return;
			}
			CPaintDC dc( this );
			CMfcPaintContext paintContext( &dc );
			pCore->Paint( &paintContext );
		}

		afx_msg void OnSize( unsigned nType, int cx, int cy )
		{
			if ( pCore->IsSettingUp() )
			{
				return;
			}
			CWnd::OnSize( nType, cx, cy );
			pCore->OnSize( cx, cy );
		}

		afx_msg void OnHScroll( unsigned nSBCode, unsigned nPos, CScrollBar* pScrollBar )
		{
			if ( pCore->IsScrollEnabled() )
			{
				CWnd::OnHScroll( nSBCode, nPos, pScrollBar );
				pCore->OnHScroll( nSBCode, nPos );
			}
		}

		afx_msg void OnVScroll( unsigned nSBCode, unsigned nPos, CScrollBar* pScrollBar )
		{
			if ( pCore->IsScrollEnabled() )
			{
				CWnd::OnVScroll( nSBCode, nPos, pScrollBar );
				pCore->OnVScroll( nSBCode, nPos );
			}
		}

		DECLARE_MESSAGE_MAP()

	public:
		// IWidget
		DECLARE_CWND_WIDGET()

		// ISceneSurface
		virtual bool Create( IFrameWindow *pFrame, CChildFrameWndBase *_pCore )
		{
			pCore = _pCore;
			return CWnd::Create( 0,
				0,
				AFX_WS_DEFAULT_VIEW,
				CRect( 0, 0, 0, 0 ),
				ToCWnd( pFrame ),
				AFX_IDW_PANE_FIRST,
				0 ) != FALSE;
		}

		virtual IWidget* GetWidget()
		{
			return this;
		}

		virtual HWND GetHandle() const
		{
			return GetSafeHwnd();
		}

		virtual void Redraw()
		{
			RedrawWindow();
		}

		virtual void RedrawWithoutErase()
		{
			RedrawWindow( 0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE );
		}

		virtual bool HasFocus() const
		{
			return ( GetSafeHwnd() != 0 ) && ( ::GetFocus() == GetSafeHwnd() );
		}

		virtual void Focus()
		{
			SetFocus();
		}

		virtual bool IsShown() const
		{
			return IsWindowVisible() != FALSE;
		}

		virtual CTPoint<int> GetClientSize() const
		{
			CRect clientRect;
			GetClientRect( &clientRect );
			return CTPoint<int>( clientRect.Width(), clientRect.Height() );
		}

		virtual void Place( const CTRect<int> &rRect )
		{
			MoveWindow( CRect( rRect.left, rRect.top, rRect.right, rRect.bottom ) );
		}

		virtual void Fill( const CTRect<int> &rRect, TWidgetColor color )
		{
			CPaintDC dc( this );
			dc.FillSolidRect( CRect( rRect.left, rRect.top, rRect.right, rRect.bottom ), static_cast<COLORREF>( color ) );
		}

		virtual void SetMouseCapture( bool bCapture )
		{
			if ( bCapture )
			{
				SetCapture();
			}
			else
			{
				ReleaseCapture();
			}
		}

		virtual bool StartUpdateTimer( int nInterval )
		{
			nUpdateSceneTimer = SetTimer( UPDATE_SCENE_TIMER_ID, nInterval, 0 );
			return nUpdateSceneTimer != 0;
		}

		virtual void StopUpdateTimer()
		{
			if ( nUpdateSceneTimer != 0 )
			{
				KillTimer( nUpdateSceneTimer );
			}
			nUpdateSceneTimer = 0;
		}

		virtual void ShowScrollBars( bool bShow )
		{
			if ( bShow )
			{
				ModifyStyle( 0, WS_VSCROLL | WS_HSCROLL, 1 );
				ShowScrollBar( SB_BOTH, true );
			}
			else
			{
				ModifyStyle( WS_VSCROLL | WS_HSCROLL, 0, 1 );
				ShowScrollBar( SB_BOTH, false );
			}
			// необходимо для того чобы показать изменения сразу
			WINDOWPLACEMENT windowPlacement;
			windowPlacement.length = sizeof( WINDOWPLACEMENT );
			//
			GetWindowPlacement( &windowPlacement );
			windowPlacement.showCmd = SW_SHOWMINIMIZED;
			SetWindowPlacement( &windowPlacement );
			RedrawWindow();
			//
			windowPlacement.showCmd = SW_SHOWMAXIMIZED;
			SetWindowPlacement( &windowPlacement );
			RedrawWindow();
		}

		virtual void SetScrollBarRange( bool bVertical, int nMin, int nMax )
		{
			SetScrollRange( bVertical ? SB_VERT : SB_HORZ, nMin, nMax, TRUE );
		}

		virtual void GetScrollBarState( bool bVertical, int *pnMin, int *pnMax, int *pnPos ) const
		{
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			const_cast<CMfcSceneSurface*>( this )->GetScrollInfo( bVertical ? SB_VERT : SB_HORZ, &si );
			( *pnMin ) = si.nMin;
			( *pnMax ) = si.nMax;
			( *pnPos ) = si.nPos;
		}

		virtual void SetScrollBarPos( bool bVertical, int nPos )
		{
			SetScrollPos( bVertical ? SB_VERT : SB_HORZ, nPos, TRUE );
		}
	};


	BEGIN_MESSAGE_MAP(CMfcSceneSurface, CWnd)
		ON_WM_CREATE()
		ON_WM_DESTROY()
		ON_WM_TIMER()
		ON_WM_SETFOCUS()
		ON_WM_KILLFOCUS()
		ON_WM_MOUSEMOVE()
		ON_WM_MOUSEWHEEL()
		ON_WM_LBUTTONDOWN()
		ON_WM_LBUTTONUP()
		ON_WM_LBUTTONDBLCLK()
		ON_WM_RBUTTONDOWN()
		ON_WM_RBUTTONUP()
		ON_WM_RBUTTONDBLCLK()
		ON_WM_MBUTTONDOWN()
		ON_WM_MBUTTONUP()
		ON_WM_MBUTTONDBLCLK()
		ON_WM_KEYDOWN()
		ON_WM_KEYUP()
		ON_WM_CHAR()
		ON_WM_SYSKEYDOWN()
		ON_WM_SYSKEYUP()
		ON_WM_SYSCHAR()
		ON_WM_CONTEXTMENU()
		ON_WM_PAINT()
		ON_WM_ERASEBKGND()
		ON_WM_SIZE()
		ON_WM_HSCROLL()
		ON_WM_VSCROLL()
	END_MESSAGE_MAP()
}


namespace NSceneSurface
{
	ISceneSurface* CreateMfc()
	{
		return new CMfcSceneSurface();
	}


	ISceneSurface* Create()
	{
		// The same flag every migrated piece follows.
		if ( NToolkit::UseWxViews() )
		{
			return CreateWx();
		}
		return CreateMfc();
	}
}
