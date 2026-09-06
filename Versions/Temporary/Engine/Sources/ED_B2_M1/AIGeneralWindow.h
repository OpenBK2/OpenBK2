#pragma once

#include "MapEditorLib/ResizeDialog.h"
#include "ResourceDefines.h"
#include "AIGeneralData.h"

#include <cstdint>

//
//
//		AI GENERAL POINTS WINDOW
//
//

class CAIGeneralPointsWindow : public CResizeDialog, public CAIGeneralPointsCommands
{
	bool bIsDataSetting;
	SAIGeneralPointsWindowData::EAIGenPointsLastAction eLastAction;
	// controls
	CListCtrl lcIDs;
	CListCtrl lcParcels;
	CComboBox	comboPlayer;
	CButton btnIDAdd;
	CButton btnIDDel;
	CButton btnParcelAdd;
	CButton btnParcelDel;

	// CResizeDialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CAIGeneralPointsWindowData )

	// CScriptAreaWindow
	void SetLastAction( const SAIGeneralPointsWindowData::EAIGenPointsLastAction eAction ) { eLastAction = eAction; }

public:
	// CPaletteCommands. Public now because the dispatch that calls them is on
	// the shared base rather than on this class.
	virtual void GetDialogData( SAIGeneralPointsWindowData *pData );
	virtual void SetDialogData( const SAIGeneralPointsWindowData *pData );

	enum { IDD = IDD_TAB_MI_AIGENERAL };
	//
	CAIGeneralPointsWindow( CWnd* pParentWindow = 0 );
	~CAIGeneralPointsWindow();

	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();
	void OnOK() {}
	void OnCancel() {}

	// HandleCommand and UpdateCommand come from CPaletteCommands, which
	// dispatches them to the two methods above.
	virtual void NotifyHandler();
	void NotifyHandler( SAIGeneralPointsWindowData::EAIGenPointsLastAction eAction );

	DECLARE_MESSAGE_MAP()
	afx_msg void OnChangePlayerCombo();
	afx_msg void OnDestroy();
	afx_msg void OnLvnItemchangedListParcels( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnLvnKeydownAigenListParcels( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnBnClickedAddParcel();
	afx_msg void OnBnClickedDeleteParcel();
	afx_msg void OnLvnItemchangedListIDs( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnLvnKeydownAigenListIDs( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnBnClickedAddID();
	afx_msg void OnBnClickedDeleteID();
	afx_msg void OnNMDblclkAigenListParcels(NMHDR *pNMHDR, LRESULT *pResult);
	//
};


