#if !defined(__MAPINFO_AI_MARKERS_DLG__)
#define __MAPINFO_AI_MARKERS_DLG__
#pragma once
#include "ResourceDefines.h"
#include "MapInfoEditorSettings.h"

//
//
//	CMapInfoAIMarkersSelectDlg
//
//

class CMapInfoAIMarkersSelectDlg : public CDialog, public ICommandHandler
{
	///
	CButton btnChkSelection;
	CComboBox cbPlayer;
	CListCtrl lcUnitTypes;
	///
	CMapInfoEditorSettings::SAIMarkerSettings *pMarkerSettings;
	int nPlayersNumber;
	///
	void SetDD();
	void GetDD();
	/// 
public:
	enum { IDD = IDD_MAPINFO_AI_MARKERS };
	///
	CMapInfoAIMarkersSelectDlg( CMapInfoEditorSettings::SAIMarkerSettings *pMarkerSettings, int nPlayersNumber );
	~CMapInfoAIMarkersSelectDlg(void);
	///
	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();
	INT_PTR DoModal();
	void OnOK();
	void OnCancel();
	///
	/// ICommandHandler
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	///
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonDefault();
};

#endif // #if !defined(__MAPINFO_AI_MARKERS_DLG__)

