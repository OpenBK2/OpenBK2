#pragma once

#include "MapEditorLib/ResizeDialog.h"
#include "EditorMethods.h"
#include "ResourceDefines.h"
#include "UnitStartCmdDialog.h"

//
//
//		UNIT START COMMAND EDITOR
//
//

class CEdUnitStartCmd : public CResizeDialog, public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CEdUnitStartCmd );

public:
	// The data and the events are the boundary's now, so that the wx dialog
	// answers in the same terms; see UnitStartCmdDialog.h. The names stay.
	typedef NUnitStartCmdDialog::SData SDlgData;

private:
	NUnitStartCmdDialog::IListener *pCommandState;
	//
	CButton btnClear;
	CComboBox cbCmdTypes;
	CEdit edData;
	CEdit edTgtUnit;
	//
	std::vector<SUnitCommandTypeInfo> cmdTypes;
	bool bEditMode;  // true - new command, false - edit command
	int nCommandIndex; // -1 -- new command

	// Resize Dialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CEdUnitStartCmd )

public:
	enum { IDD = IDD_DLG_UNIT_START_CMD };

	CEdUnitStartCmd( NUnitStartCmdDialog::IListener *pCommandState = 0 );
	virtual ~CEdUnitStartCmd() {}

	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();
	virtual INT_PTR DoModal();

	virtual void OnOK();
	virtual void OnCancel();

	void SetDialogData( const SDlgData *pData );
	void GetDialogData( SDlgData *pData );

	int GetSelectedCommandType();
	void UpdateTarget( const std::string &szNewTarget );

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedButtonClear();
	afx_msg void OnCbnSelchangeComboCmdType();
};


