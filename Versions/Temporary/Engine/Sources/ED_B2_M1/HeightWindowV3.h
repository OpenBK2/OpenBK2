#pragma once

#include "MapEditorLib/ResizeDialog.h"
#include "ResourceDefines.h"
#include "HeightStateV3.h"
#include "ED_B2_M1Dll.h"

#include <cstdint>

class CHeightWindowV3 : public CResizeDialog, public ICommandHandler
{
	static const char TILE_TYPE_NAME[];
	//
	CImageList imageList;
	//
	CButton wndBrushTileButton;
	CButton wndBrushUpButton;
	CButton wndBrushDownButton;
	CButton wndBrushRoundButton;
	CButton wndBrushPlatoButton;
	//
	CButton wndBrushSize0Button;
	CButton wndBrushSize1Button;
	CButton wndBrushSize2Button;
	CButton wndBrushSize3Button;
	CButton wndBrushSize4Button;

	CButton wndBrushTypeCircleButton;
	CButton wndBrushTypeSquareButton;
	
	CListCtrl	wndTileList;

	bool bCreateControls;
	int nStyle;
	int nLastIndex;
	vector<string> tileList;

	bool GetEditParameters( CHeightStateV3::SEditParameters *pEditParameters );
	bool SetEditParameters( const CHeightStateV3::SEditParameters &rEditParameters );

	void UpdateTileListStyle();
	void SetTileListStyle( int _nStyle );

	virtual void DoDataExchange( CDataExchange* pDX );
	//
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer( UINT nIDEvent );
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnContextMenu( CWnd *pwnd, CPoint point );
	//
	afx_msg void OnBrushRadio();
	afx_msg void OnBrushTypeRadio();
	afx_msg void OnBrushSizeRadio();
	afx_msg void OnUpdateHeights();
	afx_msg void OnItemchangedTileList( NMHDR* pNMHDR, LRESULT* pResult );

	int32_t nHeightTimer;
	uint32_t dwHeightData;
  inline UINT GetHeightID() { return 2; }
  inline UINT GetHeightTimerInterval() { return 100; }	// Частота в миллисекундах
  void SetHeightTimer();
  void KillHeightTimer();
  void OnHeightTimer();

	void UpdateSizeButtons( CHeightStateV3::SEditParameters::EBrushType eBrushType );
	
	//CResizeDialog
	bool IsToolTipsEnable() { return true; }
	bool IsDrawGripper() { return false; }

	HINSTANCE GetResourceHandle() { return theEDB2M1Instance; }

public:
	static const char FILTER_TYPE[];
	static const char EXTRACTOR_TYPE[];
	enum { IDD = IDD_TAB_MI_TERRAIN_HEIGHT_V3 };

	CHeightWindowV3( CWnd* pParent = 0 );
	~CHeightWindowV3();

	// ICommandHandler
	bool HandleCommand( UINT nCommandID, uint32_t dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
};


