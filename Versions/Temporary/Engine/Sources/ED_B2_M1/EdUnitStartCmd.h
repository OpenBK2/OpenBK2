#pragma once

#include "MapEditorLib/ResizeDialog.h"
#include "EditorMethods.h"
#include "ResourceDefines.h"

//
//
//		UNIT START COMMAND EDITOR
//
//

class CUnitStartCmdState;
class CEdUnitStartCmd : public CResizeDialog, public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CEdUnitStartCmd );

public:
	struct SDlgData
	{
		bool bEditMode; // true - new command, false - edit command
		int nSelectedCmdType;
		bool bSelectedCmdNeedTargetUnit;
		int nData;
		string szTarget;
		int nCommandIndex;
		//
		SDlgData()
		{
			Clear();
		}
		//
		void Clear()
		{
			bEditMode = true;
			nSelectedCmdType = -1;
			nData = 0;
			szTarget = "";
			bSelectedCmdNeedTargetUnit = false;
			nCommandIndex = -1;
		}
	};

private:
	CUnitStartCmdState *pCommandState;
	//
	CButton btnClear;
	CComboBox cbCmdTypes;
	CEdit edData;
	CEdit edTgtUnit;
	//
	vector<SUnitCommandTypeInfo> cmdTypes;
	bool bEditMode;  // true - new command, false - edit command
	int nCommandIndex; // -1 -- new command

	// Resize Dialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CEdUnitStartCmd )

public:
	enum { IDD = IDD_DLG_UNIT_START_CMD };

	CEdUnitStartCmd( CUnitStartCmdState *pCommandState = 0 );
	virtual ~CEdUnitStartCmd() {}

	enum EDlgEvents
	{
		EV_OK,
		EV_CANCEL,
		EV_CLEAR,
		EV_TYPE_CHANGE
	};

	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();
	virtual INT_PTR DoModal();

	virtual void OnOK();
	virtual void OnCancel();

	void SetDialogData( const SDlgData *pData );
	void GetDialogData( SDlgData *pData );

	int GetSelectedCommandType();
	void UpdateTarget( const string &szNewTarget );

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedButtonClear();
	afx_msg void OnCbnSelchangeComboCmdType();
};


