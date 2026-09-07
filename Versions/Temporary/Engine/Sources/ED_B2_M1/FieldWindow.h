#pragma once

#include "MapEditorLib/ResizeDialog.h"
#include "ResourceDefines.h"
#include "FieldState.h"

#include <cstdint>

class CFieldWindow : public CResizeDialog, public CFieldCommands
{
	bool bCreateControls;
	CComboBox	wndFieldComboBox;

protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	
	afx_msg void OnMoveRatio();
	afx_msg void OnSelchangeFieldComboBox();
	afx_msg void OnRandomizeCheckBox();
	afx_msg void OnFillTerrainCheckBox();
	afx_msg void OnFillObjectsCheckBox();
	afx_msg void OnFillHeightsCheckBox();

	//CResizeDialog
	bool IsDrawGripper() { return false; }

public:
	enum { IDD = IDD_TAB_MI_TERRAIN_FIELD };

	CFieldWindow( CWnd* pParent = 0 );
	~CFieldWindow();

	// CEditParameterCommands. Public now because the dispatch that calls them
	// is on the shared base rather than on this class.
	virtual bool GetEditParameters( CFieldState::SEditParameters *pEditParameters );
	virtual bool SetEditParameters( const CFieldState::SEditParameters &rEditParameters );

	// HandleCommand and UpdateCommand come from CEditParameterCommands, which
	// dispatches them to the two methods above.

	DECLARE_MESSAGE_MAP()
};


