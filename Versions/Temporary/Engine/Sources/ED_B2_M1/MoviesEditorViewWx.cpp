#include "stdafx.h"

#include "MoviesEditorView.h"

#ifdef OBK2_WITH_WX

#include <fmt/format.h>

#include "ED_B2_M1Dll.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/ResourceDefines.h"
#include "ResourceDefines.h"
#include "MapEditorLib/WxEditParameter.h"
#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"
#include "MovieDialogs.h"
#include "MoviesEditorData.h"
#include "SceneB2/CameraScriptMutators.h"
#include "CommandHandlerDefines.h"
#include "TimeSliderData.h"

#include <wx/bmpbuttn.h>
#include <wx/combobox.h>
#include <wx/choice.h>
#include <wx/dcbuffer.h>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/timer.h>
#include <wx/tglbtn.h>

// The script movies editor in wx: the timeline, the transport, and the mouse
// editing over the timeline.
//
// The largest thing migrated so far, and the reason it is not the hardest is
// that most of its surface was already toolkit-neutral. Everything the editor
// says to this window arrives as an ICommandHandler command on
// CHID_MOVIES_EDITOR_WINDOW, and everything it says back goes out as one to
// CHID_SCRIPT_CAMERA_STATE. Those are unchanged; what is here is the drawing,
// the controls and the mouse.
//
// Three things are worth knowing before reading it:
//
//   * **The timeline is drawn, not composed.** CTimeSliderControl paints the
//     keys, the selection, the cursor and the grid into a memory DC and blits
//     it. The panel below does the same drawing from the same data --
//     TimeSliderData.h, shared with the MFC control -- into a
//     wxAutoBufferedPaintDC, which is the same technique with the buffer
//     handled for you.
//   * **The mouse belongs to the timeline here, and to the dialog there.** The
//     MFC window handles WM_LBUTTONDOWN on itself and asks whether the point
//     is inside the slider's rectangle; the panel gets its own events and the
//     question does not arise. The rest of the logic -- shift to select, drag
//     inside a selection to move it, otherwise move the cursor -- is the same
//     and in the same order.
//   * **Setters refresh rather than repaint.** CTimeSliderControl's setters
//     call RedrawWindow, which paints synchronously, from inside whatever was
//     setting the data. That is how a stale grid spacing in a half-filled
//     control became an editor that hung for good. Refresh() asks for a paint
//     when the data has finished arriving, which is both safer and what wx
//     expects.

namespace
{
	// 0x00BBGGRR, which is what the TSL_* colours are.
	wxColour FromColorRef( COLORREF color )
	{
		return wxColour( GetRValue( color ), GetGValue( color ), GetBValue( color ) );
	}


	// A bitmap compiled into ED_B2_M1.dll, by numeric id.
	//
	// ::LoadImage rather than anything of wx's, for the reason HeightViewV3Wx.cpp
	// gives: wxBITMAP_TYPE_BMP_RESOURCE takes a resource *name* and looks in the
	// executable, and these have numeric ids in the editor DLL.
	wxBitmap LoadDllBitmap( unsigned nResourceID )
	{
		HBITMAP hBitmap = static_cast<HBITMAP>( ::LoadImage( theEDB2M1Instance,
																												 MAKEINTRESOURCE( nResourceID ),
																												 IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION ) );
		if ( hBitmap == 0 )
		{
			// A blank of the template's button size rather than a null bitmap,
			// which trips a wx assertion.
			return wxBitmap( 16, 16 );
		}
		BITMAP header = { 0 };
		if ( ::GetObject( hBitmap, sizeof( header ), &header ) == 0 )
		{
			::DeleteObject( hBitmap );
			return wxBitmap( 16, 16 );
		}
		wxBitmap bitmap;
		// Takes the handle: the wxBitmap deletes it, so hBitmap must not be.
		bitmap.InitFromHBITMAP( reinterpret_cast<WXHBITMAP>( hBitmap ),
														header.bmWidth, header.bmHeight, header.bmBitsPixel );
		return bitmap;
	}


	// What the timeline needs of the window around it. Small on purpose: the
	// panel does the mouse arithmetic and the window decides what it means.
	class ITimelineHost
	{
	public:
		virtual ~ITimelineHost() {}
		// CMoviesEditorWindow::NotifyHandler( eAction ).
		virtual void Notify( SScriptMovieEditorData::EMoviesEditorLastAction eAction ) = 0;
		// Whether there is a movie to edit, and whether it is running.
		virtual bool IsTimelineEnabled() const = 0;
		virtual bool IsPlaying() const = 0;
		// The four key commands, over the timeline.
		virtual void ShowTimelineMenu( wxWindow *pOver, const wxPoint &rAt ) = 0;
	};


	//
	//	The timeline: CTimeSliderControl's drawing and the window's mouse handling
	//

	class CTimelineWxPanel : public wxPanel
	{
		// SLI_MODE_* from MoviesEditorWindow.h, which is the MFC window's state
		// and is only ever read by the mouse handlers -- so it lives with them.
		enum EDragMode
		{
			DRAG_NOTHING			= 0x00000000,
			DRAG_MOVE_CURSOR		= 0x00000001,
			DRAG_SPECIAL_MOVE		= 0x00000002,
			DRAG_SET_SELECTION	= 0x00000004,
			DRAG_MOVE_SELECTION	= 0x00000008,
		};

		ITimelineHost *pHost = nullptr;
		float fSpacing = 1.0f;
		unsigned nDragMode = DRAG_NOTHING;

	public:
		// Public for the same reason CTimeSliderControl's is: the window fills it
		// in and reads it back.
		STimeSliderData data;

		CTimelineWxPanel( wxWindow *pParent, ITimelineHost *_pHost )
			: wxPanel( pParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN ),
				pHost( _pHost )
		{
			// SS_BLACKFRAME in the template is a filled frame, not a control that
			// paints itself; everything visible here is painted below.
			SetBackgroundStyle( wxBG_STYLE_PAINT );
			Bind( wxEVT_PAINT, &CTimelineWxPanel::OnPaint, this );
			Bind( wxEVT_SIZE, &CTimelineWxPanel::OnSize, this );
			Bind( wxEVT_LEFT_DOWN, &CTimelineWxPanel::OnLeftDown, this );
			Bind( wxEVT_LEFT_UP, &CTimelineWxPanel::OnLeftUp, this );
			Bind( wxEVT_MIDDLE_DOWN, &CTimelineWxPanel::OnMiddleDown, this );
			Bind( wxEVT_MIDDLE_UP, &CTimelineWxPanel::OnMiddleUp, this );
			Bind( wxEVT_MOTION, &CTimelineWxPanel::OnMotion, this );
			Bind( wxEVT_CONTEXT_MENU, &CTimelineWxPanel::OnContextMenu, this );
			Bind( wxEVT_MOUSE_CAPTURE_LOST, &CTimelineWxPanel::OnCaptureLost, this );
		}

		// --- the setters CMoviesEditorWindow calls on its slider ---

		void SetStartTime( float fStartTime ) { data.fStartTime = fStartTime; Refresh(); }
		void SetLength( float fLength ) { data.fLength = fLength; Refresh(); }
		void SetCursorPos( float fCursorTime ) { data.SetCursorPos( fCursorTime ); Refresh(); }
		void ResetSelection() { data.ResetSelection(); Refresh(); }
		void UpdateSelection() { data.UpdateSelection(); Refresh(); }
		void SetSelectionStart( float fStart ) { data.fSelectionStart = fStart; Refresh(); }
		void SetSelectionLength( float fLen ) { data.fSelectionLength = fLen; Refresh(); }
		void SetMoveFrom( float fMoveFrom ) { data.fMoveFromValue = fMoveFrom; Refresh(); }
		void SetMoveTo( float fMoveTo ) { data.fMoveToValue = fMoveTo; Refresh(); }
		void SetScale( float fScale ) { data.fScale = fScale; Refresh(); }
		void ClearControl() { data.Clear(); Refresh(); }

		float GetLength() const { return data.fLength; }
		float GetScale() const { return data.fScale; }
		float GetCursorPos() const { return data.GetCursorPos(); }
		float GetSelectionStart() const { return data.fSelectionStart; }
		float GetMoveOffset() const { return data.fMoveToValue - data.fMoveFromValue; }
		float GetFirstKeyTime() const { return data.GetFirstKeyTime(); }
		float GetLastKeyTime() const { return data.GetLastKeyTime(); }
		float GetPrevKeyTime() const { return data.GetPrevKeyTime(); }
		float GetNextKeyTime() const { return data.GetNextKeyTime(); }
		bool HasActiveKeys() const { return data.HasActiveKeys(); }
		bool HasSingleActiveKey() const { return data.HasSingleActiveKey(); }

		void RefreshSpacing()
		{
			fSpacing = CalcTimeSliderSpacing( data.fLength, data.fScale, GetClientSize().x );
		}

	private:
		bool HasSelection() const { return data.IsSelectionValid(); }

		int GetClientX( float fTime ) const
		{
			const float fSpan = data.fLength * data.fScale;
			if ( fSpan <= 0.0f )
			{
				return 0;
			}
			return GetClientSize().x * (fTime - data.fStartTime) / fSpan;
		}

		// CMoviesEditorWindow::GetTimeUnderCursor, with the point already local
		// to the timeline because the events arrive here.
		float GetTimeUnderCursor( int nX ) const
		{
			const int nWidth = GetClientSize().x;
			const float fSliderPart = ( nWidth > 0 ) ? Clamp( (float)nX / nWidth, 0.0f, 1.0f ) : 0.0f;
			return data.fStartTime + fSliderPart * ( data.fLength * data.fScale );
		}

		void OnSize( wxSizeEvent &rEvent )
		{
			RefreshSpacing();
			Refresh();
			rEvent.Skip();
		}

		void OnPaint( wxPaintEvent & )
		{
			// The memory DC and the blit CTimeSliderControl::OnPaint does by hand.
			wxAutoBufferedPaintDC dc( this );
			const wxSize size = GetClientSize();

			dc.SetPen( *wxTRANSPARENT_PEN );
			dc.SetBrush( wxBrush( FromColorRef( TSL_BG_COLOR ) ) );
			dc.DrawRectangle( 0, 0, size.x, size.y );

			const float fMoveOffset = GetMoveOffset();

			// selection
			if ( data.IsSelectionValid() )
			{
				const int nSelStart = GetClientX( fMoveOffset + data.fSelectionStart );
				const int nSelFinish = GetClientX( fMoveOffset + data.fSelectionStart + data.fSelectionLength );
				dc.SetBrush( wxBrush( FromColorRef( TSL_SEL_SPACE_COLOR ) ) );
				dc.DrawRectangle( nSelStart, 0, nSelFinish - nSelStart, size.y );
			}

			// cursor
			{
				const int nCursorPos = GetClientX( data.GetCursorPos() );
				dc.SetPen( wxPen( FromColorRef( TSL_SLIDER_COLOR ), TSL_DEF_CURSOR_WIDTH / 2 ) );
				dc.DrawLine( nCursorPos, 0, nCursorPos, size.y );
			}

			// grid
			if ( fSpacing > 0.0f )
			{
				dc.SetPen( wxPen( FromColorRef( TSL_GRID_COLOR ), 1 ) );
				const double fGridLines = (data.fStartTime + data.fLength * data.fScale) / fSpacing;
				const int nGridLines = ( (fGridLines > 0.0) && (fGridLines < (double)INT_MAX) ) ? (int)fGridLines : 0;
				for ( int i = 0; i < nGridLines; ++i )
				{
					const int nGridPos = GetClientX( fSpacing * i );
					if ( nGridPos > size.x )
					{
						break;
					}
					if ( nGridPos >= 0 )
					{
						dc.DrawLine( nGridPos, size.y * 3 / 4, nGridPos, size.y );
					}
				}
			}

			// keys
			{
				dc.SetBackgroundMode( wxTRANSPARENT );
				for ( std::vector<SMovieKeyData>::const_iterator itKey = data.keys.begin();
							itKey != data.keys.end(); ++itKey )
				{
					if ( itKey->bActive )
					{
						const int nKeyPos = GetClientX( fMoveOffset + itKey->fTime );
						if ( (nKeyPos >= 0) && (nKeyPos <= size.x) )
						{
							dc.SetPen( wxPen( FromColorRef( TSL_AKEY_COLOR ), 3 ) );
							dc.DrawLine( nKeyPos, 0, nKeyPos, size.y );
							dc.SetTextForeground( FromColorRef( TSL_AKEY_COLOR ) );
							dc.DrawText( wxString::FromUTF8( itKey->szCameraName.c_str() ), nKeyPos + 2, 0 );
						}
					}
					else
					{
						const int nKeyPos = GetClientX( itKey->fTime );
						if ( (nKeyPos >= 0) && (nKeyPos <= size.x) )
						{
							dc.SetPen( wxPen( FromColorRef( TSL_KEY_COLOR ), 3 ) );
							dc.DrawLine( nKeyPos, 0, nKeyPos, size.y );
						}
					}
				}
			}

			// outer timeline: everything past the end of the movie
			{
				const int nEnd = GetClientX( data.fLength );
				dc.SetPen( *wxTRANSPARENT_PEN );
				dc.SetBrush( wxBrush( FromColorRef( TSL_BG_E_COLOR ) ) );
				dc.DrawRectangle( nEnd, 0, size.x, size.y );
			}
		}

		// --- the mouse, from CMoviesEditorWindow ---

		void OnLeftDown( wxMouseEvent &rEvent )
		{
			if ( !pHost->IsTimelineEnabled() )
			{
				rEvent.Skip();
				return;
			}
			CaptureMouse();
			if ( pHost->IsPlaying() )
			{
				pHost->Notify( SScriptMovieEditorData::ME_PAUSE );
			}
			const float fTime = GetTimeUnderCursor( rEvent.GetX() );

			if ( rEvent.ShiftDown() )
			{
				// change selection start and finish
				SetSelectionStart( fTime );
				SetSelectionLength( DEF_SEL_RAD );
				UpdateSelection();
				nDragMode |= DRAG_SET_SELECTION;
			}
			else if ( HasSelection() && data.IsPointInsideSelection( fTime ) )
			{
				// move selection
				SetMoveFrom( fTime );
				SetMoveTo( fTime );
				nDragMode |= DRAG_MOVE_SELECTION;
			}
			else
			{
				// just change time
				ResetSelection();
				UpdateSelection();
				SetCursorPos( fTime );
				pHost->Notify( SScriptMovieEditorData::ME_CHANGE_TIME );
				pHost->Notify( SScriptMovieEditorData::ME_CLEAR_MARKERS );
				nDragMode |= DRAG_MOVE_CURSOR;
			}
		}

		void OnLeftUp( wxMouseEvent & )
		{
			if ( HasCapture() )
			{
				ReleaseMouse();
			}
			if ( !pHost->IsTimelineEnabled() )
			{
				return;
			}
			if ( pHost->IsPlaying() )
			{
				pHost->Notify( SScriptMovieEditorData::ME_PLAY );
			}
			if ( nDragMode & DRAG_SET_SELECTION )
			{
				nDragMode &= ~DRAG_SET_SELECTION;
			}
			if ( nDragMode & DRAG_MOVE_SELECTION )
			{
				SetSelectionStart( GetSelectionStart() + GetMoveOffset() );
				pHost->Notify( SScriptMovieEditorData::ME_MOVE_KEYS );
				SetMoveFrom( 0.0f );
				SetMoveTo( 0.0f );
				nDragMode &= ~DRAG_MOVE_SELECTION;
			}
			if ( nDragMode & DRAG_MOVE_CURSOR )
			{
				if ( !pHost->IsPlaying() )
				{
					pHost->Notify( SScriptMovieEditorData::ME_DRAW_MARKERS );
				}
				pHost->Notify( SScriptMovieEditorData::ME_SELECT_CAMERA );
				nDragMode &= ~DRAG_MOVE_CURSOR;
			}
		}

		void OnMotion( wxMouseEvent &rEvent )
		{
			if ( !pHost->IsTimelineEnabled() )
			{
				rEvent.Skip();
				return;
			}
			const float fCursorTime = GetTimeUnderCursor( rEvent.GetX() );

			if ( nDragMode & DRAG_MOVE_CURSOR )
			{
				ResetSelection();
				SetCursorPos( fCursorTime );
				pHost->Notify( SScriptMovieEditorData::ME_CHANGE_TIME );
			}
			if ( nDragMode & DRAG_SPECIAL_MOVE )
			{
				ResetSelection();
				SetCursorPos( fCursorTime );
				UpdateSelection();
			}
			if ( nDragMode & DRAG_SET_SELECTION )
			{
				SetSelectionLength( fCursorTime - GetSelectionStart() );
				SetCursorPos( fCursorTime );
				UpdateSelection();
			}
			if ( nDragMode & DRAG_MOVE_SELECTION )
			{
				SetMoveTo( fCursorTime );
			}
		}

		void OnMiddleDown( wxMouseEvent &rEvent )
		{
			if ( !pHost->IsTimelineEnabled() )
			{
				rEvent.Skip();
				return;
			}
			CaptureMouse();
			// special move cursor: moves the cursor and takes the keys under it
			// without telling the state until the button comes up
			const float fTime = GetTimeUnderCursor( rEvent.GetX() );
			ResetSelection();
			SetCursorPos( fTime );
			UpdateSelection();
			nDragMode |= DRAG_SPECIAL_MOVE;
		}

		void OnMiddleUp( wxMouseEvent & )
		{
			if ( HasCapture() )
			{
				ReleaseMouse();
			}
			if ( !pHost->IsTimelineEnabled() )
			{
				return;
			}
			pHost->Notify( SScriptMovieEditorData::ME_SELECT_CAMERA );
			nDragMode &= ~DRAG_SPECIAL_MOVE;
		}

		void OnCaptureLost( wxMouseCaptureLostEvent & )
		{
			// wx insists this is handled; the drag simply ends where it is.
			nDragMode = DRAG_NOTHING;
		}

		void OnContextMenu( wxContextMenuEvent &rEvent )
		{
			pHost->ShowTimelineMenu( this, ScreenToClient( rEvent.GetPosition() ) );
		}
	};


	//
	//	The window: the transport, the lists, and the command handler
	//

	class CMoviesEditorWxWindow : public CWxHostWindow, public ICommandHandler, public ITimelineHost
	{
		// The bitmaps and the buttons, in the MFC window's order; SB_PAUSE is a
		// bitmap with no button of its own.
		enum EButton
		{
			SB_JUMP_FIRST = 0,
			SB_JUMP_LAST,
			SB_STEP_PREV,
			SB_STEP_NEXT,
			SB_STOP,
			SB_PLAY,
			SB_PAUSE,
			SB_ADD_SEQ,
			SB_DEL_SEQ,
			SB_SETUP,
			SB_RESIZE_WND,
			SB_SIZE
		};

		CTimelineWxPanel *pTimeline = nullptr;
		wxSlider *pTimeSlider = nullptr;
		wxSlider *pScaleSlider = nullptr;
		wxComboBox *pMovieCombo = nullptr;
		wxChoice *pSpeedCombo = nullptr;
		wxTextCtrl *pTimeEdit = nullptr;
		wxBitmapButton *pButtons[SB_SIZE] = { nullptr };
		wxBitmapToggleButton *pResizeToGame = nullptr;
		wxBitmap bitmaps[SB_SIZE];
		wxTimer timer;

		bool bIsDataSetting = false;
		bool bIsSliderEnabled = false;
		bool bIsPlaying = false;

		SScriptMovieEditorData::EMoviesEditorLastAction eLastAction =
			SScriptMovieEditorData::ME_NO_ACTIONS;
		SScriptMovieEditorData dialogData;
		float fNewLength = 0.0f;

	public:
		CMoviesEditorWxWindow()
		{
			// As the MFC window's constructor does, so that commands sent before
			// the window is built still find a handler.
			Singleton<ICommandHandlerContainer>()->Set( CHID_MOVIES_EDITOR_WINDOW, this );
		}

		virtual ~CMoviesEditorWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->Remove( CHID_MOVIES_EDITOR_WINDOW );
		}

		bool Build( IWidget *pPane )
		{
			Singleton<ICommandHandlerContainer>()->Set( CHID_MOVIES_EDITOR_WINDOW, this );
			Singleton<ICommandHandlerContainer>()->Register( CHID_MOVIES_EDITOR_WINDOW,
																											ID_MIMOVED_INSERT_KEY, ID_MIMOVED_DELETE_KEYS );
			if ( !CreateHost( pPane ) )
			{
				return false;
			}
			wxWindow *const pRoot = Root();
			LoadBitmaps();

			// Two rows, as the template has them: the timeline and the transport
			// above, the two sliders and the lists below. The anchors in
			// CMoviesEditorWindow's constructor say which parts take the width --
			// the timeline on the first row, the sliders on the second at 0.7 and
			// 0.3 -- and that is what the proportions below are.
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			// The pane is taller than the strip of controls, and every one of them
			// is anchored to its bottom -- ANCHORE_*_BOTTOM in the MFC window's
			// constructor -- so the empty space is above them, not around them.
			pSizer->AddStretchSpacer( 1 );

			wxBoxSizer *pTopRow = new wxBoxSizer( wxHORIZONTAL );
			pTimeline = NWx::Child<CTimelineWxPanel>( pRoot, this );
			// 20 dialog units tall, as the template's IDC_DMOVED_TIME_SLIDER_PLACE
			// is; it takes the width and not the height.
			pTimeline->SetMinSize( wxSize( -1, pRoot->ConvertDialogToPixels( wxSize( 0, 20 ) ).y ) );
			pTopRow->Add( pTimeline, wxSizerFlags( 1 ).Expand().Border( wxRIGHT, 4 ) );
			pTimeEdit = NWx::Child<wxTextCtrl>( pRoot, wxID_ANY, "0", wxDefaultPosition,
																					wxSize( 56, -1 ), wxTE_READONLY );
			pTopRow->Add( pTimeEdit, wxSizerFlags().CentreVertical().Border( wxRIGHT, 4 ) );
			// NOT WS_VISIBLE in the template: the stop button is made and never
			// shown, and the play button does both jobs.
			pButtons[SB_STOP] = AddButton( pRoot, pTopRow, SB_STOP, "Stop movie" );
			pButtons[SB_STOP]->Hide();
			pButtons[SB_JUMP_FIRST] = AddButton( pRoot, pTopRow, SB_JUMP_FIRST, "Jump first key" );
			pButtons[SB_STEP_PREV] = AddButton( pRoot, pTopRow, SB_STEP_PREV, "Step previous key" );
			pButtons[SB_PLAY] = AddButton( pRoot, pTopRow, SB_PLAY, "Start-pause movie" );
			pButtons[SB_STEP_NEXT] = AddButton( pRoot, pTopRow, SB_STEP_NEXT, "Step next key" );
			pButtons[SB_JUMP_LAST] = AddButton( pRoot, pTopRow, SB_JUMP_LAST, "Jump last key" );
			pSizer->Add( pTopRow, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT | wxTOP, 2 ) );

			wxBoxSizer *pBottomRow = new wxBoxSizer( wxHORIZONTAL );
			pTimeSlider = NWx::Child<wxSlider>( pRoot, wxID_ANY, 0, 0, 100 );
			pBottomRow->Add( pTimeSlider, wxSizerFlags( 7 ).CentreVertical() );
			pScaleSlider = NWx::Child<wxSlider>( pRoot, wxID_ANY, 100, 0, 100 );
			pBottomRow->Add( pScaleSlider, wxSizerFlags( 3 ).CentreVertical().Border( wxRIGHT, 4 ) );
			pBottomRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Speed:" ),
											 wxSizerFlags().CentreVertical().Border( wxRIGHT, 2 ) );
			pSpeedCombo = NWx::Child<wxChoice>( pRoot, wxID_ANY, wxDefaultPosition, wxSize( 56, -1 ) );
			pBottomRow->Add( pSpeedCombo, wxSizerFlags().CentreVertical().Border( wxRIGHT, 4 ) );
			pBottomRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Movie:" ),
											 wxSizerFlags().CentreVertical().Border( wxRIGHT, 2 ) );
			// CBS_DROPDOWN | CBS_SORT: it has an edit field, and it is sorted, so
			// the tenth movie sits between the first and the second. The window
			// reads the *position* back as the movie number, which is the same
			// thing only while there are fewer than ten -- kept as it is.
			pMovieCombo = NWx::Child<wxComboBox>( pRoot, wxID_ANY, wxString(), wxDefaultPosition,
																						wxSize( 56, -1 ), 0, nullptr, wxCB_SORT );
			pBottomRow->Add( pMovieCombo, wxSizerFlags().CentreVertical().Border( wxRIGHT, 4 ) );
			pButtons[SB_ADD_SEQ] = AddButton( pRoot, pBottomRow, SB_ADD_SEQ, "Add sequence" );
			pButtons[SB_DEL_SEQ] = AddButton( pRoot, pBottomRow, SB_DEL_SEQ, "Delete sequence" );
			pButtons[SB_SETUP] = AddButton( pRoot, pBottomRow, SB_SETUP, "Settings" );
			// BS_AUTOCHECKBOX | BS_PUSHLIKE | BS_BITMAP: a button that stays in.
			pResizeToGame = NWx::Child<wxBitmapToggleButton>( pRoot, wxID_ANY, bitmaps[SB_RESIZE_WND],
																												wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
			pBottomRow->Add( pResizeToGame, wxSizerFlags().CentreVertical() );
			pSizer->Add( pBottomRow, wxSizerFlags().Expand().Border( wxLEFT | wxRIGHT | wxBOTTOM, 2 ) );

			pRoot->SetSizer( pSizer );

			pButtons[SB_JUMP_FIRST]->Bind( wxEVT_BUTTON, &CMoviesEditorWxWindow::OnJumpFirstKey, this );
			pButtons[SB_JUMP_LAST]->Bind( wxEVT_BUTTON, &CMoviesEditorWxWindow::OnJumpLastKey, this );
			pButtons[SB_STEP_PREV]->Bind( wxEVT_BUTTON, &CMoviesEditorWxWindow::OnStepPrevKey, this );
			pButtons[SB_STEP_NEXT]->Bind( wxEVT_BUTTON, &CMoviesEditorWxWindow::OnStepNextKey, this );
			pButtons[SB_STOP]->Bind( wxEVT_BUTTON, &CMoviesEditorWxWindow::OnStopMovie, this );
			pButtons[SB_PLAY]->Bind( wxEVT_BUTTON, &CMoviesEditorWxWindow::OnPlayPauseMovie, this );
			pButtons[SB_ADD_SEQ]->Bind( wxEVT_BUTTON, &CMoviesEditorWxWindow::OnAddSeq, this );
			pButtons[SB_DEL_SEQ]->Bind( wxEVT_BUTTON, &CMoviesEditorWxWindow::OnDelSeq, this );
			pButtons[SB_SETUP]->Bind( wxEVT_BUTTON, &CMoviesEditorWxWindow::OnSettings, this );
			pResizeToGame->Bind( wxEVT_TOGGLEBUTTON, &CMoviesEditorWxWindow::OnResizeToGame, this );
			pMovieCombo->Bind( wxEVT_COMBOBOX, &CMoviesEditorWxWindow::OnMovieChanged, this );
			pSpeedCombo->Bind( wxEVT_CHOICE, &CMoviesEditorWxWindow::OnSpeedChanged, this );
			pTimeSlider->Bind( wxEVT_SLIDER, &CMoviesEditorWxWindow::OnSliderScroll, this );
			pScaleSlider->Bind( wxEVT_SLIDER, &CMoviesEditorWxWindow::OnSliderScroll, this );
			timer.Bind( wxEVT_TIMER, &CMoviesEditorWxWindow::OnTimerTick, this );

			pTimeline->SetStartTime( 0.0f );

			// The speed list, -10 to +10, starting at 0 in the middle.
			std::vector<std::string> speeds;
			for ( int nAnimSpeedIndex = -10; nAnimSpeedIndex <= 10; ++nAnimSpeedIndex )
			{
				if ( nAnimSpeedIndex > 0 )
					speeds.push_back( fmt::format( "+{}", nAnimSpeedIndex ) );
				else
					speeds.push_back( std::to_string( nAnimSpeedIndex ) );
			}
			NWxEditParameter::WriteChoice( pSpeedCombo, speeds, 10, true, true );
			return true;
		}

		void Unregister()
		{
			Singleton<ICommandHandlerContainer>()->UnRegister( CHID_MOVIES_EDITOR_WINDOW );
			Singleton<ICommandHandlerContainer>()->Remove( CHID_MOVIES_EDITOR_WINDOW );
		}

		//	ICommandHandler -- the channel everything else reaches this window by

		virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData )
		{
			switch ( nCommandID )
			{
				case ID_WINDOW_GET_DIALOG_DATA:
				{
					SScriptMovieEditorData *pData = reinterpret_cast<SScriptMovieEditorData*>( dwData );
					NI_VERIFY( pData, "CMoviesEditorWxWindow::HandleCommand(): dwData == 0", return false );
					GetDialogData( pData );
					return true;
				}
				case ID_WINDOW_SET_DIALOG_DATA:
				{
					SScriptMovieEditorData *pData = reinterpret_cast<SScriptMovieEditorData*>( dwData );
					NI_VERIFY( pData, "CMoviesEditorWxWindow::HandleCommand(): dwData == 0", return false );
					SetDialogData( *pData );
					return true;
				}
				case ID_MOV_ED_SET_TIMER:
				{
					timer.Start( MOVIE_TIMER_INTERVAL );
					return true;
				}
				case ID_MOV_ED_KILL_TIMER:
				case ID_MOV_ED_RESET_DIALOG:
				{
					// Both of these only stop the timer in the MFC window; its
					// ResetDialog, which puts the play bitmap back, is called by
					// nothing.
					timer.Stop();
					return true;
				}
				case ID_MIMOVED_INSERT_KEY:
				{
					Notify( SScriptMovieEditorData::ME_INSERT_KEY );
					return true;
				}
				case ID_MIMOVED_SAVE_KEY:
				{
					Notify( SScriptMovieEditorData::ME_SAVE_KEY );
					return true;
				}
				case ID_MIMOVED_KEY_SETTINGS:
				{
					Notify( SScriptMovieEditorData::ME_KEY_SETTINGS );
					return true;
				}
				case ID_MIMOVED_DELETE_KEYS:
				{
					Notify( SScriptMovieEditorData::ME_DELETE_KEYS );
					return true;
				}
			}
			return false;
		}

		virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
		{
			NI_ASSERT( pbEnable != 0, "CMoviesEditorWxWindow::UpdateCommand(), pbEnable == 0" );
			NI_ASSERT( pbCheck != 0, "CMoviesEditorWxWindow::UpdateCommand(), pbCheck == 0" );
			switch ( nCommandID )
			{
				case ID_MIMOVED_INSERT_KEY:
					( *pbEnable ) = true;
					( *pbCheck ) = false;
					return true;
				case ID_MIMOVED_SAVE_KEY:
				case ID_MIMOVED_KEY_SETTINGS:
					( *pbEnable ) = pTimeline != nullptr && pTimeline->HasSingleActiveKey();
					( *pbCheck ) = false;
					return true;
				case ID_MIMOVED_DELETE_KEYS:
					( *pbEnable ) = pTimeline != nullptr && pTimeline->HasActiveKeys();
					( *pbCheck ) = false;
					return true;
				default:
					return false;
			}
		}

		//	ITimelineHost

		virtual void Notify( SScriptMovieEditorData::EMoviesEditorLastAction eAction )
		{
			eLastAction = eAction;
			if ( !bIsDataSetting )
			{
				CWaitCursor wcur;
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_CAMERA_STATE,
																															ID_SCRIPT_CAMERA_MOV_ED_UI_EVENT, 0 );
			}
			eLastAction = SScriptMovieEditorData::ME_NO_ACTIONS;
		}

		virtual bool IsTimelineEnabled() const { return bIsSliderEnabled; }
		virtual bool IsPlaying() const { return bIsPlaying; }

		virtual void ShowTimelineMenu( wxWindow *pOver, const wxPoint &rAt )
		{
			// IDM_MAPINFO_CONTEXT_MENU's MI_MOVIES_EDITOR popup, as a wxMenu. The
			// MFC window tracks the resource menu with the frame as its owner, so
			// the frame routes the click back here through the command handler
			// container; this asks the container directly, which is the same
			// journey with one fewer toolkit in it. The enabled states come from
			// UpdateCommand, which is what the frame's idle handler asks too.
			wxMenu menu;
			AppendCommand( &menu, ID_MIMOVED_INSERT_KEY, "Insert Key" );
			AppendCommand( &menu, ID_MIMOVED_SAVE_KEY, "Save Key" );
			AppendCommand( &menu, ID_MIMOVED_KEY_SETTINGS, "Key Settings" );
			menu.AppendSeparator();
			AppendCommand( &menu, ID_MIMOVED_DELETE_KEYS, "Delete keys" );
			pOver->PopupMenu( &menu, rAt );
			// As the MFC window does when its menu closes.
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
			pTimeline->UpdateSelection();
		}

	private:
		static const int MOVIE_TIMER_INTERVAL = 100;

		void AppendCommand( wxMenu *pMenu, unsigned nCommandID, const char *pszLabel )
		{
			bool bEnable = false;
			bool bCheck = false;
			UpdateCommand( nCommandID, &bEnable, &bCheck );
			wxMenuItem *const pItem = pMenu->Append( nCommandID, wxString::FromUTF8( pszLabel ) );
			pItem->Enable( bEnable );
			pMenu->Bind( wxEVT_MENU,
									 [this, nCommandID]( wxCommandEvent & )
									 {
										 Singleton<ICommandHandlerContainer>()->HandleCommand(
											 CHID_MOVIES_EDITOR_WINDOW, nCommandID, 0 );
									 },
									 nCommandID );
		}

		void LoadBitmaps()
		{
			bitmaps[SB_JUMP_FIRST] = LoadDllBitmap( IDB_DMOVED_JUMP_FIRST_KEY );
			bitmaps[SB_JUMP_LAST] = LoadDllBitmap( IDB_DMOVED_JUMP_LAST_KEY );
			bitmaps[SB_STEP_PREV] = LoadDllBitmap( IDB_DMOVED_STEP_PREV_KEY );
			bitmaps[SB_STEP_NEXT] = LoadDllBitmap( IDB_DMOVED_STEP_NEXT_KEY );
			bitmaps[SB_STOP] = LoadDllBitmap( IDB_DMOVED_STOP_MOVIE );
			bitmaps[SB_PLAY] = LoadDllBitmap( IDB_DMOVED_PLAY_MOVIE );
			// Loaded and never used, exactly as in the MFC window: the button
			// shows the *stop* bitmap while a movie is running.
			bitmaps[SB_PAUSE] = LoadDllBitmap( IDB_DMOVED_PAUSE_MOVIE );
			bitmaps[SB_ADD_SEQ] = LoadDllBitmap( IDB_DMOVED_ADD_SEQ );
			bitmaps[SB_DEL_SEQ] = LoadDllBitmap( IDB_DMOVED_DEL_SEQ );
			bitmaps[SB_SETUP] = LoadDllBitmap( IDB_DMOVED_SETTINGS );
			bitmaps[SB_RESIZE_WND] = LoadDllBitmap( IDB_DMOVED_CHECK_RESIZE_WND );
		}

		wxBitmapButton* AddButton( wxWindow *pParent, wxSizer *pRow, int nButton, const char *pszTip )
		{
			wxBitmapButton *const pButton =
				NWx::Child<wxBitmapButton>( pParent, wxID_ANY, bitmaps[nButton], wxDefaultPosition,
																		wxDefaultSize, wxBU_EXACTFIT );
			pButton->SetToolTip( wxString::FromUTF8( pszTip ) );
			pRow->Add( pButton, wxSizerFlags().CentreVertical() );
			return pButton;
		}

		//	the data, in and out

		void GetDialogData( SScriptMovieEditorData *pDialogData )
		{
			pDialogData->Clear();
			( *pDialogData ) = dialogData;

			pDialogData->nActiveMovie = pMovieCombo->GetSelection();
			pDialogData->fCursorTime = pTimeline->GetCursorPos();
			pDialogData->eLastAction = eLastAction;
			pDialogData->nSpeed = pSpeedCombo->GetSelection() - 10;

			pDialogData->activeKeysList.SetSize( pTimeline->data.keys.size() );
			pDialogData->activeKeysList.FillZero();
			int i = 0;
			for ( std::vector<SMovieKeyData>::const_iterator itKey = pTimeline->data.keys.begin();
						itKey != pTimeline->data.keys.end(); ++itKey, ++i )
			{
				if ( itKey->bActive )
				{
					pDialogData->activeKeysList.SetData( i );
				}
			}
			pDialogData->fMoveValue = pTimeline->GetMoveOffset();
			pDialogData->fNewLength = fNewLength;
		}

		void SetDialogData( const SScriptMovieEditorData &rDialogData )
		{
			bIsDataSetting = true;

			dialogData = rDialogData;
			const bool bSequenceExists = ( dialogData.scriptMoviesData.scriptMovieSequences.size() > 0 );

			pTimeSlider->Enable( bSequenceExists );
			pScaleSlider->Enable( bSequenceExists );
			pMovieCombo->Enable( bSequenceExists );
			pButtons[SB_JUMP_FIRST]->Enable( bSequenceExists );
			pButtons[SB_JUMP_LAST]->Enable( bSequenceExists );
			pButtons[SB_STEP_PREV]->Enable( bSequenceExists );
			pButtons[SB_STEP_NEXT]->Enable( bSequenceExists );
			pButtons[SB_STOP]->Enable( bSequenceExists );
			pButtons[SB_PLAY]->Enable( bSequenceExists );
			pButtons[SB_DEL_SEQ]->Enable( bSequenceExists );
			pButtons[SB_SETUP]->Enable( bSequenceExists );

			// movie list
			pMovieCombo->Clear();
			int i = -1;
			for ( i = 0; i < static_cast<int>( dialogData.scriptMoviesData.scriptMovieSequences.size() ); ++i )
			{
				pMovieCombo->Append( wxString::FromUTF8( fmt::format( "{}", i ).c_str() ) );
			}
			if ( ( dialogData.nActiveMovie <= i ) && ( dialogData.nActiveMovie >= 0 ) )
			{
				SelectMovie( fmt::format( "{}", dialogData.nActiveMovie ) );
			}
			else if ( i > 0 )
			{
				SelectMovie( "0" );
			}

			// timeline
			if ( ( dialogData.nActiveMovie >= 0 ) &&
					 ( dialogData.nActiveMovie < static_cast<int>( dialogData.scriptMoviesData.scriptMovieSequences.size() ) ) )
			{
				bIsSliderEnabled = true;
				const NDb::SScriptMovieSequence &seq =
					dialogData.scriptMoviesData.scriptMovieSequences[dialogData.nActiveMovie];
				pTimeline->SetLength( seq.GetLength() );

				pTimeline->data.keys.resize( 0 );
				for ( std::vector<NDb::SScriptMovieKeyPos>::const_iterator itKey = seq.posKeys.begin();
							itKey != seq.posKeys.end(); ++itKey )
				{
					SMovieKeyData newKey;
					newKey.fTime = itKey->fStartTime;
					newKey.szCameraName =
						dialogData.scriptMoviesData.scriptCameraPlacements[itKey->nPositionIndex].szName;
					pTimeline->data.keys.push_back( newKey );
				}
				pTimeline->RefreshSpacing();
				pTimeline->UpdateSelection();
			}
			else
			{
				pTimeline->ClearControl();
				bIsSliderEnabled = false;
			}

			pTimeline->Enable( bIsSliderEnabled );

			bIsDataSetting = false;
			pTimeline->Refresh();
		}

		// CComboBox::SelectString, which matches on a prefix; this matches the
		// whole string, which for a list of movie numbers is the same answer.
		void SelectMovie( const std::string &rszText )
		{
			const int nItem = pMovieCombo->FindString( wxString::FromUTF8( rszText.c_str() ) );
			if ( nItem != wxNOT_FOUND )
			{
				pMovieCombo->SetSelection( nItem );
			}
		}

		void UpdateDialogData()
		{
			if ( bIsPlaying )
			{
				const CScriptMoviesMutatorHolder *pMoviesHolder = Camera()->GetScriptMutatorsHolder();
				if ( pMoviesHolder )
				{
					pTimeline->SetCursorPos( pMoviesHolder->GetTime() );
				}
			}
			pTimeEdit->ChangeValue(
				wxString::FromUTF8( fmt::format( "{:g}", pTimeline->GetCursorPos() ).c_str() ) );
		}

		//	the transport

		void OnTimerTick( wxTimerEvent & ) { UpdateDialogData(); }

		void OnJumpFirstKey( wxCommandEvent & ) { JumpTo( pTimeline->GetFirstKeyTime() ); }
		void OnJumpLastKey( wxCommandEvent & ) { JumpTo( pTimeline->GetLastKeyTime() ); }
		void OnStepPrevKey( wxCommandEvent & ) { JumpTo( pTimeline->GetPrevKeyTime() ); }
		void OnStepNextKey( wxCommandEvent & ) { JumpTo( pTimeline->GetNextKeyTime() ); }

		void JumpTo( float fTime )
		{
			pTimeline->ResetSelection();
			pTimeline->SetCursorPos( fTime );
			Notify( SScriptMovieEditorData::ME_CHANGE_TIME );
			Notify( SScriptMovieEditorData::ME_SELECT_CAMERA );
		}

		void OnAddSeq( wxCommandEvent & ) { Notify( SScriptMovieEditorData::ME_ADD_SEQ ); }
		void OnDelSeq( wxCommandEvent & ) { Notify( SScriptMovieEditorData::ME_DEL_SEQ ); }

		void OnStopMovie( wxCommandEvent & )
		{
			Notify( SScriptMovieEditorData::ME_STOP );
			pButtons[SB_PLAY]->SetBitmap( bitmaps[SB_PLAY] );
			// Toggled, not cleared, exactly as the MFC window does it -- pressing
			// Stop while stopped leaves this thinking a movie is running.
			bIsPlaying = !bIsPlaying;
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		}

		void OnPlayPauseMovie( wxCommandEvent & )
		{
			if ( bIsPlaying )
			{
				Notify( SScriptMovieEditorData::ME_STOP );
				pButtons[SB_PLAY]->SetBitmap( bitmaps[SB_PLAY] );
			}
			else
			{
				Notify( SScriptMovieEditorData::ME_PLAY );
				pButtons[SB_PLAY]->SetBitmap( bitmaps[SB_STOP] );
			}
			bIsPlaying = !bIsPlaying;
		}

		void OnSettings( wxCommandEvent & )
		{
			if ( ( dialogData.nActiveMovie >= 0 ) &&
					 ( dialogData.nActiveMovie < static_cast<int>( dialogData.scriptMoviesData.scriptMovieSequences.size() ) ) )
			{
				fNewLength =
					dialogData.scriptMoviesData.scriptMovieSequences[dialogData.nActiveMovie].GetLength();
			}
			// Which toolkit draws it is NMovieSettings' business.
			if ( NMovieSettings::Run( Singleton<IMainFrameContainer>()->GetMainWindow(), &fNewLength ) )
			{
				Notify( SScriptMovieEditorData::ME_RESIZE );
			}
		}

		void OnResizeToGame( wxCommandEvent & )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_RESIZE_TO_GAME,
																														pResizeToGame->GetValue() );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		}

		void OnMovieChanged( wxCommandEvent & )
		{
			dialogData.nActiveMovie = pMovieCombo->GetSelection();
			Notify( SScriptMovieEditorData::ME_MOVIE_SWITCH );
		}

		void OnSpeedChanged( wxCommandEvent & )
		{
			Notify( SScriptMovieEditorData::ME_CHANGE_SPEED );
		}

		// OnHScroll: both sliders arrive here, as they do in MFC.
		void OnSliderScroll( wxCommandEvent & )
		{
			pTimeline->SetStartTime( ( pTimeline->GetLength() * pTimeline->GetScale() ) *
															 SliderPercent( pTimeSlider ) );
			pTimeline->SetScale( pow( MOVED_DEF_SCALING, ( SliderPercent( pScaleSlider ) - 1.0f ) *
																MOVED_DEF_SCALING ) );
			pTimeline->RefreshSpacing();
			Notify( SScriptMovieEditorData::ME_CHANGE_TIME );
		}

		static float SliderPercent( const wxSlider *pSlider )
		{
			const int nRange = pSlider->GetMax() - pSlider->GetMin();
			return ( nRange != 0 ) ? (float)pSlider->GetValue() / nRange : 0.0f;
		}

		static const float MOVED_DEF_SCALING;
	};

	const float CMoviesEditorWxWindow::MOVED_DEF_SCALING = 2.0f;


	//
	//	The view the pane is given
	//

	class CWxMoviesEditorView : public NMoviesEditorView::IView
	{
		CMoviesEditorWxWindow window;
		bool bCreated = false;

	public:
		virtual ~CWxMoviesEditorView()
		{
			Destroy();
		}

		virtual bool Create( IWidget *pPane )
		{
			bCreated = window.Build( pPane );
			return bCreated;
		}

		virtual void Destroy()
		{
			if ( bCreated )
			{
				window.Unregister();
				window.DestroyHost();
				bCreated = false;
			}
		}

		virtual void Show( bool bShow )
		{
			if ( bCreated )
			{
				window.ShowHost( bShow );
			}
		}

		virtual IWidget* GetWidget()
		{
			return bCreated ? &window : 0;
		}
	};
}


namespace NMoviesEditorView
{
	IView* CreateWx()
	{
		return new CWxMoviesEditorView;
	}
}

#endif // OBK2_WITH_WX
