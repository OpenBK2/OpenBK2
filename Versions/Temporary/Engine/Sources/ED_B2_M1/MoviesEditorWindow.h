#pragma once

#include "ResourceDefines.h"
#include "../MapEditorLib/ResizeDialog.h"
#include "../MapEditorLib/Interface_CommandHandler.h"
#include "MoviesEditorData.h"
#include "TimeSliderControl.h"

#define DEF_TIMELINE_LEN	1000

#define SLI_MODE_NOTHING				0x00000000
#define SLI_MODE_MOVE_CURSOR		0x00000001
#define SLI_MODE_SPECIAL_MOVE		0x00000002
#define SLI_MODE_SET_SELECTION	0x00000004
#define SLI_MODE_MOVE_SELECTION	0x00000008

#define MOVIE_IS_STOPPED	0x00000000
#define MOVIE_IS_PLAYING	0x00000001

//
//
//		MOVIES EDITOR WINDOW
//
//

class CMoviesEditorWindow : public CResizeDialog, public ICommandHandler
{
	enum ESliderButton
	{
		SB_JUMP_FIRST = 0,
		SB_JUMP_LAST,
		SB_STEP_PREV,
		SB_STEP_NEXT,
		SB_STOP,
		SB_PLAY,			// these two bitmaps are
		SB_PAUSE,			// used for one button!
		SB_ADD_SEQ,
		SB_DEL_SEQ,
		SB_SETUP,
		SB_RESIZE_WND,
		SB_SIZE
	};

	CPoint lastPoint;

	bool bIsDataSetting;
	bool bIsSliderEnabled;
	UINT nSliderMode;
	UINT nMovieMode;

	SScriptMovieEditorData::EMoviesEditorLastAction eLastAction;
	SScriptMovieEditorData dialogData;

	float fNewLength;
	LONG nMovieTimerID;

	// CResizeDialog
	CBitmap bitmaps[SB_SIZE];
	CButton buttons[SB_SIZE];

	CSliderCtrl wndSliderTime;
	CSliderCtrl wndSliderScale;
	CComboBox wndMovieCombo;
	CComboBox	wndSpeedCombo;
	CEdit wndTimeEdit;
	CTimeSliderControl wndTimeSliderControl;

	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CMoviesEditorWindow )

	virtual void OnOK() {};
	virtual void OnCancel() {};

	void SetLastAction( SScriptMovieEditorData::EMoviesEditorLastAction eAction ) { eLastAction = eAction; }
	void GetTimeSliderRect( CRect* pRect ) const;
	float GetTimeUnderCursor( const CRect rect, const CPoint point ) const
	{
		const float fSliderPart = Clamp( (float)(point.x) / rect.Width(), 0.0f, 1.0f );
		return wndTimeSliderControl.data.fStartTime + fSliderPart *
					 (wndTimeSliderControl.data.fLength * wndTimeSliderControl.data.fScale);
	}

	bool HasSelection() const { return wndTimeSliderControl.data.IsSelectionValid(); }

public:
	enum { IDD = IDD_DLG_MOVIES_EDITOR };

	CMoviesEditorWindow( CWnd *pParentWindow = 0 );
	virtual ~CMoviesEditorWindow();

	bool Create( CWnd *pParentWindow );
	void Destroy();
	virtual BOOL OnInitDialog();

	void DoDataExchange( CDataExchange *pDX );

	void GetDialogData( SScriptMovieEditorData *pDialogData );
	void SetDialogData( const SScriptMovieEditorData &rDialogData );
	void UpdateDialogData();
	void ResetDialog();

	// ICommandHandler
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	virtual void NotifyHandler();
	virtual void NotifyHandler( SScriptMovieEditorData::EMoviesEditorLastAction eAction );

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedJumpFirstKey();
	afx_msg void OnBnClickedJumpLastKey();
	afx_msg void OnBnClickedStepPrevKey();
	afx_msg void OnBnClickedStepNextKey();
	afx_msg void OnBnClickedStopMovie();
	afx_msg void OnBnClickedPlayPauseMovie();
	afx_msg void OnBnClickedAddSeq();
	afx_msg void OnBnClickedDelSeq();
	afx_msg void OnBnClickedSettings();
	afx_msg void OnBnClickedDmovedResize();
	afx_msg void OnCbnSelchangeComboMovie();
	afx_msg void OnCbnSelChangeTimerSpeed();
	afx_msg void OnPaint();
	afx_msg void OnTimer( UINT nIDEvent );
	afx_msg void OnLButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnLButtonUp( UINT nFlags, CPoint point );
	afx_msg void OnMouseMove( UINT nFlags, CPoint point );
	afx_msg void OnMButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnMButtonUp( UINT nFlags, CPoint point );
	afx_msg BOOL OnMouseWheel( UINT nFlags, short zDelta, CPoint pt );

	afx_msg void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar *pScrollBar );
	afx_msg void OnContextMenu( CWnd *pWnd, CPoint point );

	inline UINT GetMovieTimerID() { return 3; }
	inline UINT GetMovieTimerInterval() { return 100; }	// timer update interval
	void SetMovieTimer();
	void KillMovieTimer();
};


