#pragma once

#include "ResourceDefines.h"

#include "PC_BaseDialog.h"
#include "MapEditorLib/ResizeDialog.h"
#include "PC_MainTreeControl.h"
#include "MapEditorLib/Interface_Builder.h"

#define PC_BD_TREE_COLUMN_COUNT (3)

class CPCBuildDataDialog : public CResizeDialog, public CPCBaseDialog
{
	static const unsigned	PC_BD_TREE_COLUMN_NAME  [PC_BD_TREE_COLUMN_COUNT];
	static const int	PC_BD_TREE_COLUMN_FORMAT[PC_BD_TREE_COLUMN_COUNT];
	static const int	PC_BD_TREE_COLUMN_WIDTH [PC_BD_TREE_COLUMN_COUNT];

	bool bCreateControls;
	ICommandHandler *pPreviousCommandHandler;
	//
	SBuildDataParams *pBuildDataParams;
	struct IBuildDataCallback *pBuildDataCallback;
	//
	CImageList typesImageList;
	CImageList headerImageList;
	//
	CPCMainTreeControl tree;
	CStatic	wndTreeStatusStringWindow;
	//
	void InitImageLists();
	
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	//
	afx_msg void OnDestroy();
	afx_msg void OnChangeNameEdit();
	afx_msg void OnNeedExportCheckBox();
	afx_msg LRESULT OnMessagePCManipulatorChange( WPARAM wParam, LPARAM lParam );
	//
	// CResizeDialog
	int GetMinimumXDimension() { return 300; }
	int GetMinimumYDimension() { return 200; }
	void GetXMLFilePath( string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CPCBuildDataDialog"; }
	bool IsDrawGripper() { return true; }

	void UpdateOKButton();
	void UpdateTitle();

public:
	enum { IDD = IDD_PC_BD };
	//
	CPCBuildDataDialog( CWnd* pParent = NULL );
	//	
	void SetBuildDataParams( SBuildDataParams *_pBuildDataParams ) { pBuildDataParams = _pBuildDataParams; }
	void SetTemporaryLabel( const string &rszTemporaryLabel ) { tree.SetTemporaryLabel( rszTemporaryLabel ); }
	void SetBuildDataCallback( struct IBuildDataCallback *_pBuildDataCallback ) { pBuildDataCallback = _pBuildDataCallback; }
	//
	//CPCBaseDialog
	IView* GetView();
	ICommandHandler* GetCommandHandler();
	void CreateTree();
	void UpdateValues();
	void EnableEdit( bool bEnable ) { tree.EnableEdit( bEnable ); }

	DECLARE_MESSAGE_MAP()
};


