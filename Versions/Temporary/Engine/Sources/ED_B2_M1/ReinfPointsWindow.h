#pragma once

#include "MapEditorLib/Interface_Widget.h"

#include "MapEditorLib/ResizeDialog.h"
#include "ReinfPointsData.h"
#include "ResourceDefines.h"

#include <cstdint>

//
//
//						REINFPOINTS WINDOW
//
//

class CReinfPointsWindow : public CResizeDialog, public CReinfPointsCommands
{
protected:
	//
	CListCtrl lcReinfPoints;
	CComboBox	wndPlayerComboBox;
	CButton btnDel;
	CButton btnDeploy;
	CButton btnTyped;

	bool bIsDataSetting;
	bool bIsAvia;
	int nSelectedIndex;
	SReinfPointsWindowData::EReinfWndLastAction eLastAction;
	//
	void NotifyHandler();

	// Enter, Leave and Draw were declared here and never defined anywhere, so
	// they are gone rather than carried across.

public:
	// CPaletteCommands. Public now because the dispatch that calls them is on
	// the shared base rather than on this class.
	virtual void GetDialogData( SReinfPointsWindowData *pData );
	virtual void SetDialogData( const SReinfPointsWindowData *pData );

	enum { IDD = IDD_TAB_MI_REINF_POINTS };

	CReinfPointsWindow( CWnd* pParentWindow = 0 );
	virtual ~CReinfPointsWindow();

	BOOL OnInitDialog();
	void DoDataExchange( CDataExchange *pDX );

	int GetMinimumXDimension() { return 100; }
	int GetMinimumYDimension() { return 130; }
	bool IsDrawGripper() { return false; }

	virtual void OnKeyDown(unsigned nChar, unsigned nRepCnt, unsigned nFlags);

	virtual void OnOK() {}
	virtual void OnCancel() {}

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnCbnSelchangeComboPlayer();
	afx_msg void OnBnClickedButtonReinfPointsAdd();
	afx_msg void OnBnClickedButtonReinfPointsDel();
	afx_msg void OnBnClickedButtonReinfPointsDeploy();
	afx_msg void OnBnClickedButtonReinfPointsTyped();
	afx_msg void OnNMClickListReinfPoints(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedListReinfPoints(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnDblclkListReinfPoints(NMHDR *pNMHDR, LRESULT *pResult);
};


